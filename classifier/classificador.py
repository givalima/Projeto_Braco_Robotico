#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tvieira@ic.ufal.br
"""

# Importing the Keras libraries and packages
 
from keras.models import Sequential
from keras.layers import Conv2D
from keras.layers import MaxPooling2D
from keras.layers import Flatten
from keras.layers import Dense
from keras.preprocessing.image import ImageDataGenerator
import numpy as np

# Initialising the CNN
classifier = Sequential()
classifier.add(Conv2D(32, (3, 3), input_shape=(50, 50, 1), activation='relu'))
classifier.add(MaxPooling2D(pool_size=(2, 2)))
classifier.add(Conv2D(32, (3, 3), activation='relu'))
classifier.add(MaxPooling2D(pool_size=(2, 2)))
classifier.add(Flatten())
classifier.add(Dense(units=120, activation='relu'))
classifier.add(Dense(units=84, activation='relu'))
classifier.add(Dense(units=2, activation='softmax'))
classifier.compile(optimizer='adam', loss='categorical_crossentropy',
                   metrics=['accuracy'])

classifier.summary()

# Part 2 - Fitting the CNN to the images

dist_dir = 'C:/Users/Givanildo Lima/Documents/imagens/folders/f4_binarizada/'
#depth_dir = '../Gestures/static_poses_dist/F1/'

iFold = 1
train_datagen = ImageDataGenerator(rescale=1./255)

test_datagen = ImageDataGenerator(rescale=1./255)

training_set = train_datagen.flow_from_directory(
  dist_dir +'train',
  target_size=(50, 50),
  color_mode='grayscale',
  batch_size=32,
  class_mode='categorical')

test_set = test_datagen.flow_from_directory(
  dist_dir + 'test',
  target_size=(50, 50),
  color_mode='grayscale',
  batch_size=32,
  class_mode='categorical',
  shuffle=False)

# Fit the classifier

classifier.fit_generator(training_set,
                         steps_per_epoch=40,
                         epochs=25,
                         validation_data=test_set,
                         validation_steps=test_set.samples/32)
score = classifier.predict_generator(test_set)
print(score.shape) 

classifier.save("parametros")
print("Parametros atualizados")


# %%%%%%  MATRIZ DE CONFUSÃO  %%%%%%%
							 
cut_pos = int(len(test_set.classes) / len(test_set.class_indices))

w, h = 2, 2;
mat = [[0 for x in range(w)] for y in range(h)] 
k = 0
for i in range(0, 69):
    k = np.argmax(score[i])
    mat[0][k] += 1

for i in range(69, 133):
    k = np.argmax(score[i])
    mat[1][k] += 1

print(mat)
