import socket
import mysql.connector
from mysql.connector import Error
from datetime import datetime

DB_HOST = "server01-mysql"
DB_USER = "myuser"
DB_PASSWORD = "my-secret-pw"
DB_NAME = "gp_db"
PORT = 8084

def add_user_to_database(conn, username, points, datetime_str):
    insert_query = "INSERT INTO usertable (username, userpoints, datetime_stamps) VALUES (%s, %s, %s)"
    values = (username, points, datetime_str)
    try:
        cursor = conn.cursor()
        cursor.execute(insert_query, values)
        conn.commit()
        cursor.close()
        print("User added successfully")
    except Error as e:
        print(f"Error adding user to the database: {e}")

def delete_user_from_database(conn, username):
    count_query = "SELECT COUNT(*) FROM usertable WHERE username = %s"
    values = (username,)
    try:
        cursor = conn.cursor()
        cursor.execute(count_query, values)
        user_count = cursor.fetchone()[0]
        if user_count > 0:
            delete_query = "DELETE FROM usertable WHERE username = %s"
            cursor.execute(delete_query, values)
            conn.commit()
            print("User deleted successfully")
        else:
            print("NO USER IN THE DATABASE")
        cursor.close()
    except Error as e:
        print(f"Error deleting user from the database: {e}")

def show_database(conn):
    select_query = "SELECT * FROM usertable"
    try:
        cursor = conn.cursor()
        cursor.execute(select_query)
        rows = cursor.fetchall()
        num_fields = len(cursor.description)
        print("\nDatabase Contents:")
        print("---------------------------------")
        for row in rows:
            for i in range(num_fields):
                print(row[i] if row[i] else "NULL", end="\t")
            print()
        print("---------------------------------")
        cursor.close()
    except Error as e:
        print(f"Error retrieving data from the database: {e}")

def get_current_datetime():
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('', PORT))
    server_socket.listen(3)

    print("Server is running...")

    conn = None
    try:
        conn = mysql.connector.connect(
            host=DB_HOST,
            user=DB_USER,
            password=DB_PASSWORD,
            database=DB_NAME
        )
        cursor = conn.cursor()
        cursor.execute("CREATE TABLE IF NOT EXISTS usertable (username VARCHAR(50), userpoints INT, datetime_stamps DATETIME)")
        cursor.close()

        while True:
            client_socket, address = server_socket.accept()
            print("Client connected")

            request = client_socket.recv(256).decode()
            print("Received request:", request)

            tokens = request.split(" ")
            if len(tokens) > 0:
                if tokens[0] == "ADD_USER":
                    if len(tokens) == 3:
                        username = tokens[1]
                        points = int(tokens[2])
                        datetime_str = get_current_datetime()
                        add_user_to_database(conn, username, points, datetime_str)
                        response = "User added successfully"
                    else:
                        response = "Invalid request"
                elif tokens[0] == "DELETE_USER":
                    if len(tokens) == 2:
                        username = tokens[1]
                        delete_user_from_database(conn, username)
                        response = "User deleted successfully"
                    else:
                        response = "Invalid request"
                elif tokens[0] == "SHOW_DB":
                    show_database(conn)
                    response = "Database contents sent"
                elif tokens[0] == "END_CONN":
                    response = "Connection ended"
                    client_socket.close()
                    break
                else:
                    response = "Invalid request"
            else:
                response = "Invalid request"

            client_socket.send(response.encode())
            print("Response sent:", response)

            client_socket.close()
            print("Client disconnected")

    except Error as e:
        print(f"Error connecting to the database: {e}")
    finally:
        if conn is not None and conn.is_connected():
            conn.close()

    server_socket.close()

if __name__ == "__main__":
    main()