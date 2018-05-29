# -*- coding: utf-8 -*-
"""
Created on Tue May 15 08:40:02 2018

@author: Givanildo Lima
"""

import cv2
import os

img_aberta = cv2.imread('C://Users//Givanildo Lima//Documents//imagens//binarizada//mao_aberta//binarizada73.ppm', 0)

img_fechada = cv2.imread('C://Users//Givanildo Lima//Documents//imagens//binarizada//mao_fechada//binarizada100.ppm', 0)

def calc_black_percent(img):
    #print(img.shape)
    im = img.reshape((img.shape[0] * img.shape[1]))
    cont = 0

    for pixel in im:
        if(pixel == 0):
            cont += 1
            
    percent = cont*100/len(im)
    return percent

black_percent_aberta = calc_black_percent(img_aberta)
black_percent_fechada = calc_black_percent(img_fechada)


lista_aberta = os.listdir('C://Users//Givanildo Lima//Documents//imagens//binarizada//mao_aberta')
lista_fechada = os.listdir('C://Users//Givanildo Lima//Documents//imagens//binarizada//mao_fechada')

lista_aberta = [os.path.join('C://Users//Givanildo Lima//Documents//imagens//binarizada//mao_aberta', l) for l in lista_aberta]
lista_fechada = [os.path.join('C://Users//Givanildo Lima//Documents//imagens//binarizada//mao_fechada', l) for l in lista_fechada]

aberta = 0
fechada = 0

for i in lista_aberta:
    img = cv2.imread(i, 0)
    if not(calc_black_percent(img) >= black_percent_aberta *0.5 and calc_black_percent(img) <= black_percent_aberta * 1.5):
        os.remove(i)
        aberta += 1
        
for i in lista_fechada:
   img = cv2.imread(i, 0)
   if not(calc_black_percent(img) >= black_percent_fechada *0.5 and calc_black_percent(img) <= black_percent_fechada * 1.5):
       os.remove(i)
       fechada += 1
       
print("\nABERTA:", aberta, "FECHADA:", fechada)
        
#cv2.destroyAllWindows()