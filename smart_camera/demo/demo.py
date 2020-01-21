import socket
import threading
import time
import sys
import cv2
import numpy
result_list = []
def str_to_hex(s):
        return ''.join([hex(ord(c)).replace('0x','  ')for c in s])

def socket_service():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('192.168.100.100', 8888))
        s.listen(10)
    except socket.error as msg:
        print(msg)
        sys.exit(1)
    print('Waiting connection...')
 
    while 1:
        conn, addr = s.accept()
        t = threading.Thread(target=deal_data, args=(conn, addr))
        t.start()
 
def deal_data(conn, addr):
    global result_list
    print('Accept new connection from {0}'.format(addr))
    while 1:
        data = conn.recv(1024)
        #print('{0} client send data is {1}'.format(addr, data.decode()))#b'\xe8\xbf\x99\xe6\xac\xa1\xe5\x8f\xaf\xae4\xbb\xa5\xe4\xba\x86'
        data_list = (str_to_hex(data)).split(" ")
        print data_list[2],data_list[16]
        print data_list[68],data_list[70],data_list[72],data_list[74]
        x0 = int(data_list[72],16) * 256 + int(data_list[74],16)
        print data_list[76],data_list[78],data_list[80],data_list[82]
        y0 = int(data_list[80],16) * 256 + int(data_list[82],16)
        print data_list[84],data_list[86],data_list[88],data_list[90]
        x1 = int(data_list[88],16) * 256 + int(data_list[90],16)
        print data_list[92],data_list[94],data_list[96],data_list[98]
        y1 = int(data_list[96],16) * 256 + int(data_list[98],16)

        alert = int(data_list[28],16)
        print data_list[100]
        #print x0,y0,x1,y1
        result_list = [x0,y0,x1,y1,alert]
        print result_list
        #time.sleep(1)
        if data == 'exit' or not data:
            print('{0} connection close'.format(addr))
            break
    conn.close()
 
width = 640
height = 480

if __name__ == '__main__':
    global result_list
    ttt = threading.Thread(target=socket_service)
    ttt.start()

    cap = cv2.VideoCapture(0)
    cap.set(3,width)
    cap.set(4,height)
    while 1:
        ret, frame = cap.read()
        cv2.line(frame,(0*width/300, 100*height/300),(50*width/300,300*height/300),(255,0,0),1)
        cv2.line(frame,(0*width/300, 100*height/300),(100*width/300,300*height/300),(0,255,0),1)
        cv2.line(frame,(0*width/300, 100*height/300),(150*width/300,300*height/300),(0,0,255),1)
        #cv2.line(frame,(43*width/300, 178*height/300),(65*width/300,300*height/300),(0,0,255),1)
        #cv2.line(frame,(53*width/300, 178*height/300),(105*width/300,300*height/300),(0,255,0),1)
        #cv2.line(frame,(59*width/300, 178*height/300),(147*width/300,300*height/300),(255,0,0),1)
        if result_list:
            if result_list[4] == 3:
                cv2.rectangle(frame,(result_list[0]*width/300,result_list[1]*height/300),(result_list[2]*width/300,result_list[3]*height/300),(255,0,0),2)
            elif result_list[4] == 2:
                cv2.rectangle(frame,(result_list[0]*width/300,result_list[1]*height/300),(result_list[2]*width/300,result_list[3]*height/300),(0,255,0),2)
            elif result_list[4] == 1:
                cv2.rectangle(frame,(result_list[0]*width/300,result_list[1]*height/300),(result_list[2]*width/300,result_list[3]*height/300),(0,0,255),2)
            elif result_list[4] == 0:
                cv2.rectangle(frame,(result_list[0]*width/300,result_list[1]*height/300),(result_list[2]*width/300,result_list[3]*height/300),(255,255,0),2)
            result_list = []
        cv2.imshow("cap", frame)
        if cv2.waitKey(100) & 0xff == ord('q'):
            break
    cap.release()
    cv2.destroyAllWindows()

