import socket

PORT = 8084
SERVER_IP = "172.19.0.3"

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((SERVER_IP, PORT))
        print("Connected to the server")

        while True:
            print("\nOptions:")
            print("1. Add User")
            print("2. Delete User")
            print("3. Show Database")
            print("4. End Connection")

            option = input("\nEnter an option: ")

            if option == "1":
                username = input("Enter the username: ")
                points = input("Enter the points: ")
                request = f"ADD_USER {username} {points}"
                sock.send(request.encode())

                response = sock.recv(256).decode()
                print("Server response:", response)

            elif option == "2":
                username = input("Enter the username: ")
                request = f"DELETE_USER {username}"
                sock.send(request.encode())

                response = sock.recv(256).decode()
                print("Server response:", response)

            elif option == "3":
                request = "SHOW_DB"
                sock.send(request.encode())

                print("\nServer response:")
                while True:
                    response = sock.recv(256).decode()
                    if response == "Database contents sent":
                        break
                    print(response)

            elif option == "4":
                request = "END_CONN"
                sock.send(request.encode())
                break

            else:
                print("Invalid option. Please try again.")

    except ConnectionRefusedError:
        print("Connection to the server refused")

    finally:
        sock.close()

if __name__ == "__main__":
    main()