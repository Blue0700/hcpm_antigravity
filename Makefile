CC = gcc
CFLAGS = -Wall -O2 -std=c99 -Iinclude
TARGET = hcpm_test
SRCDIR = src
OBJS = $(SRCDIR)/main.o $(SRCDIR)/modulator.o $(SRCDIR)/demodulator.o $(SRCDIR)/viterbi.o $(SRCDIR)/channel.o $(SRCDIR)/utils.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)
