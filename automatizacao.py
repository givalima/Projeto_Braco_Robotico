import os
import numpy as np
import shutil
import cv2 as cv

def convert_and_copy_ppm_to_png(src, dest):
  im = cv.imread(src)
  cv.imwrite(dest, im)

pasta_classe_1_origem = 'mao_aberta'
pasta_classe_2_origem = 'mao_fechada'
pasta_classe_1_dest = 'm1'
pasta_classe_2_dest = 'm2'
test_ratio = 0.1;
np.random.seed(0)

def func_mao_aberta(pasta_origem, folder_destino):
	dir_name_first = 'C:/Users/Givanildo Lima/Documents/imagens/' + pasta_origem + '/' + pasta_classe_1_origem
	dir_name_secon = 'C:/Users/Givanildo Lima/Documents/imagens/folders/' + folder_destino

	if os.path.exists(dir_name_secon):
		shutil.rmtree(dir_name_secon) 
	os.mkdir(dir_name_secon)
	os.mkdir(dir_name_secon+'/train')
	os.mkdir(dir_name_secon+'/test')
	os.mkdir(dir_name_secon+'/train/'+ pasta_classe_1_dest)
	os.mkdir(dir_name_secon+'/train/'+ pasta_classe_2_dest)
	os.mkdir(dir_name_secon+'/test/'+ pasta_classe_1_dest)
	os.mkdir(dir_name_secon+'/test/'+ pasta_classe_2_dest)


	ims = os.listdir(dir_name_first)
	total_array = np.arange(len(ims))
	np.random.shuffle(total_array)

	seq_train = total_array[0: int(len(total_array) * (1.0-test_ratio))]
	seq_test = total_array[len(total_array) - int(len(total_array) * test_ratio): len(total_array)]

	print("copia train - ", pasta_origem, " mao aberta")

	for key, val in enumerate(seq_train):
		file_name = sorted(os.listdir(dir_name_first), key=len)[val]
		convert_and_copy_ppm_to_png(os.path.join(dir_name_first, file_name), os.path.join(dir_name_secon ,'train', pasta_classe_1_dest, file_name[:len(file_name) - 4] + '.png'))

	print("copia test - ", pasta_origem, " mao aberta")

	for key, val in enumerate(seq_test):
		file_name = sorted(os.listdir(dir_name_first), key=len)[val]
		convert_and_copy_ppm_to_png(os.path.join(dir_name_first, file_name), os.path.join(dir_name_secon ,'test', pasta_classe_1_dest, file_name[:len(file_name) - 4] + '.png'))

def func_mao_fechada(pasta_origem, folder_destino):
	dir_name_first = 'C:/Users/Givanildo Lima/Documents/imagens/' + pasta_origem + '/'+ pasta_classe_2_origem
	dir_name_secon = 'C:/Users/Givanildo Lima/Documents/imagens/folders/' + folder_destino

	ims = os.listdir(dir_name_first)
	total_array = np.arange(len(ims))
	np.random.shuffle(total_array)

	seq_train = total_array[0: int(len(total_array) * (1.0-test_ratio))]
	seq_test = total_array[len(total_array) - int(len(total_array) * test_ratio): len(total_array)]

	print("copia train - ", pasta_origem, " mao fechada")

	for key, val in enumerate(seq_train):
		file_name = sorted(os.listdir(dir_name_first), key=len)[val]
		convert_and_copy_ppm_to_png(os.path.join(dir_name_first, file_name), os.path.join(dir_name_secon ,'train', pasta_classe_2_dest, file_name[:len(file_name) - 4] + '.png'))

	print("copia test - ", pasta_origem, "mao fechada")

	for key, val in enumerate(seq_test):
		file_name = sorted(os.listdir(dir_name_first), key=len)[val]
		convert_and_copy_ppm_to_png(os.path.join(dir_name_first, file_name), os.path.join(dir_name_secon ,'test', pasta_classe_2_dest, file_name[:len(file_name) - 4] + '.png'))

#################      RGB      #################

func_mao_aberta("rgb", "f1_rgb")
func_mao_fechada("rgb", "f1_rgb")


#################      PROFUNDIDADE      #################

func_mao_aberta("profundidade", "f2_profundidade")
func_mao_fechada("profundidade", "f2_profundidade")

#################      SEGMENTADA     #################

func_mao_aberta("segmentada", "f3_segmentada")
func_mao_fechada("segmentada", "f3_segmentada")

#################      BINARIZADA     #################

func_mao_aberta("binarizada", "f4_binarizada")
func_mao_fechada("binarizada", "f4_binarizada")