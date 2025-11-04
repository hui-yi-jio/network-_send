CC = clang 
CFLAGS = -std=gnu2y -O3 -ffast-math -march=native
LDFLAGS = -lm

TARGET = net

$(TARGET): net.o
