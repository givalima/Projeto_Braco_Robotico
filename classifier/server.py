# -*- coding: utf-8 -*-
"""
@author: Tiago vieira
@author: Thales vieira
@author: Givanildo Lima
@author: Lucas Amaral
"""
from keras.models import load_model
import socket
import struct
import numpy as np
import scipy.misc
from scripts.RobotArm import RobotArm


# %%%%%%%  SERVIDOR  %%%%%%%%

def server():
    open_conn = True

    while True:
        classifier = load_model("parametros.hdf5")
        tcp_ip = '127.0.0.1'
        tcp_port = 31000
        buffer_size = 50*4
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((tcp_ip, tcp_port))
        s.listen(1)
        print('Waiting connection…')
        conn, addr = s.accept()
        print('Connection address:', addr)
        robot_arm = RobotArm()
        center = [0, 0, 0]
        robot_arm.set_map_transform([1, 1, 1], center)

        while 1:
            current_size = 0
            buffer = b""

            [x, y, z] = struct.unpack('3f', conn.recv(4*3))

            #[xa, ya, za] = struct.unpack('3f', conn.recv(4*3))

            will_get_hand = struct.unpack('f', conn.recv(4))[0]

            print('[kinect]({} {} {})'.format(x, y, z))
#            print('({} {} {})'.format(xa, ya, za), file=open('arms.txt', 'a'))

            # y kinect == horizontal ao robo
            robot_arm.set_p_rel_kinect([z, x, y])

            if will_get_hand == 1:
                for i in range(50):
                    data = conn.recv(buffer_size)
                    if not data:
                        conn.close()
                        open_conn = False
                        break

                    buffer += data
                    current_size += len(data)
                
                if not open_conn:
                    open_conn = True
                    break

                imgint = struct.unpack("2500i", buffer)

                npimg = np.array(imgint).reshape(50, 50)
                sciimg = scipy.misc.toimage(npimg)
                imglow = sciimg.resize((50, 50))

                npimglow = np.array(imglow).reshape(1,50,50,1)
                # PREDICT DA IMAGEM
                predvec = classifier.predict(npimglow)

                robot_arm.set_gripper_state(np.argmax(predvec) * 2)
                predvecstr = "%f" % predvec[0][0]
                for i in range(1, len(predvec[0])):
                    predvecstr = "%s %f" % (predvecstr,predvec[0][i])
                conn.send(predvecstr.encode())


server()
