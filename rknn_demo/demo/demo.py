import cv2
import thread
import socket
tuple_data = tuple()
SERVER_ADDR='192.168.101.155'
RTSP_ADDR="rtsp://%s:8554/live"%SERVER_ADDR
flag = 0
def socket_run():
    global tuple_data
    global flag
    client = socket.socket()
    client.connect((SERVER_ADDR,8889))
    while True:
        data = client.recv(64)
        #print('recv:',data.decode('utf-8'))
        #print('recv:',data)
        tuple_data = tuple(eval(data))
        #print(tuple_data[4])
        flag = 1
    client.close()
thread.start_new_thread(socket_run,())
print(RTSP_ADDR)
cap = cv2.VideoCapture(RTSP_ADDR)
ret,frame = cap.read()
while ret:
    ret,frame = cap.read()
    cv2.line(frame,(0,150),(200,480),(0,255,0),1)
    if flag:
        #print("draw the kuangzi")
        print(tuple_data[1],tuple_data[2],tuple_data[3],tuple_data[4])
        if 33*tuple_data[1]/20 + 150 > tuple_data[4]:
            cv2.rectangle(frame,(tuple_data[1],tuple_data[2],tuple_data[3],tuple_data[4]),(255,0,0),3)
        else:
            cv2.rectangle(frame,(tuple_data[1],tuple_data[2],tuple_data[3],tuple_data[4]),(0,0,255),3)
        cv2.putText(frame,tuple_data[0],(tuple_data[1]+4,tuple_data[2]+18),cv2.FONT_HERSHEY_SIMPLEX,0.8,(0,255,0),1)
        flag = 0
    cv2.imshow("frame",frame)
    if cv2.waitKey(1) & 0xff == ord('q'):
        break
cv2.destoryAllWindows()
cap.release()

