CC = g++

TARGET = main

all: 
	@$(CC) producer.cpp -g -o producer
	@$(CC) consumer.cpp -g -o consumer