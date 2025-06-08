import socket

def send_resp_command(command):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('localhost', 6379))
    
    s.sendall(command)
    
    response = s.recv(1024)
    print("Response:", response.decode())
    
    s.close()

def test_commands():
    print("\nTesting SET command...")
    set_cmd = b'*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n'
    send_resp_command(set_cmd)
    
    print("\nTesting GET command...")
    get_cmd = b'*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n'
    send_resp_command(get_cmd)
    
    print("\nTesting DEL command...")
    del_cmd = b'*2\r\n$3\r\nDEL\r\n$3\r\nkey\r\n'
    send_resp_command(del_cmd)
    
    print("\nTesting GET after DEL...")
    send_resp_command(get_cmd)

if __name__ == "__main__":
    test_commands() 