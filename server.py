# -*- coding: utf-8 -*-
"""
Created on Thu Apr 12 12:59:09 2018

@author: Givanildo Lima
"""
from keras.models import load_model
import socket
import struct
import numpy as np
import scipy.misc


# %%%%%%%  SERVIDOR  %%%%%%%%

def server():
    
    open_conn = True
    while True:
        classifier = load_model("parametros")
        TCP_IP = '127.0.0.1'
        TCP_PORT = 31000
        BUFFER_SIZE = 50*4
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((TCP_IP, TCP_PORT))
        s.listen(1)
        print('Waiting connection…')
        conn, addr = s.accept() #CONECTOU
        print('Connection address:', addr)
    
    
        while 1: # WHILE INFINITO PARA SEMPRE ESTAR RECEBENDO IMAGENS
            current_size = 0
            size = 50*50*4 # width * heigth * sizeof(unsigned int) #1228800
            buffer = b""
    
            for i in range(50): #while current_size < size: #WHILE PARA RECEBER UMA IMAGEM, UMA LINHA DE CADA VEZ
                data = conn.recv(BUFFER_SIZE)
                #print(len(data))
                if not data:
                    conn.close()
                    open_conn = False
                    break
                #data = data[:size-current_size]
                #conn.send('ok'.encode())
                #print(data)
                buffer += data
                current_size += len(data)
                #print('end')
                
            if not open_conn:
                open_conn = True
                break
           
            #print(len(buffer))
            # CONVERTE A IMAGEM PARA O FORMATO DO KERAS
            imgint = struct.unpack("2500i", buffer)   #"307200I",buffer)
            
            npimg=np.array(imgint).reshape(50,50)
            sciimg = scipy.misc.toimage(npimg)
            imglow=sciimg.resize((50,50))
            npimglow = np.array(imglow).reshape(1,50,50,1)
            # PREDICT DA IMAGEM
            predvec = classifier.predict(npimglow)
    
            predvecstr = "%f" % predvec[0][0]
            for i in range(1, len(predvec[0])):
                predvecstr = "%s %f" % (predvecstr,predvec[0][i])
            conn.send(predvecstr.encode())

server()
conn.close()