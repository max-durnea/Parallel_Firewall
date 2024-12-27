# A parallel firewall that implements syncronisation and multithreading using pthread library
### It has a producer and a number of consumers. The consumers analyze packets and write the result to a file.
### I didn't implement timestamp ordering because the program would always break
