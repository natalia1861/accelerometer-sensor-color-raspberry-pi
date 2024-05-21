/* UDP client in the internet domain */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define I2C_DEVICE "/dev/i2c-1"
#define SLAVE_ACC_ADDR 0x68 //7-bit Slave address
#define SLAVE_COLOUR_ADDR 0x29 //7-bit Slave address

void write_reg(int slave_adrr, int fd, unsigned char reg_dir,
		unsigned char write_data);
void read_reg(int slave_adrr, int fd, unsigned char reg_dir,
		unsigned char *read_data, int r_len);
void init_accelerometer(int fd);
void init_colour_sensor(int fd);

void error(const char*);

typedef struct {
	float x;
	float y;
	float z;
	float red;
	float green;
	float blue;
} client_msg;

int main(int argc, char *argv[]) {
	//net variables
	int sock, n;
	unsigned int length;
	struct sockaddr_in server, from;
	struct hostent *hp;
	uint8_t buffer[256];
	uint8_t numero = 1;
	//sensor variables
	int fd_acc;
	int fd_colour;
	unsigned char acc_value[6];
	unsigned char colour_data[6];
	client_msg message[10];

//region
	//iniciamos el servidor
	if (argc != 3) {
		printf("Usage: server port\n");
		exit(1);
	}
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		error("socket");

	//server code (copy)
	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp == 0)
		error("Unknown host");

	bcopy((char*) hp->h_addr,
	(char *)&server.sin_addr,
	hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	length = sizeof(struct sockaddr_in);

	printf("Initiating sensors...\n");

	//iniciamos los sensores
	//OPEN THE I2C DEVICE for accelerometer
	fd_acc = open(I2C_DEVICE, O_RDWR);	//Obtain file descriptor for RW
	if (fd_acc < 0) {
		printf("failed to open file descriptor to i2c. ACCELEROMETER");
		exit(1);
	}

	//OPEN I2C DEVICE for COLOUR SENSOR
	fd_colour = open(I2C_DEVICE, O_RDWR);	//Obtain file descriptor for RW
	if (fd_acc < 0) {
		printf("failed to open file descriptor to i2c. COLOUR");
		exit(1);
	}

	//Configure the file for I2C communications with slave
	if (ioctl(fd_acc, I2C_SLAVE, SLAVE_ACC_ADDR) < 0) {
		printf(
				"failed to configure the file for I2C ACCELEROMETER communication.");
		exit(1);
	}

	//set i2c slave address
	if (ioctl(fd_acc, I2C_SLAVE, SLAVE_ACC_ADDR) < 0) {
		perror("Failed to set the I2C slave ACCELEROMETER address");
		exit(1);
	}

	//Configure the file for I2C communications with slave
	if (ioctl(fd_colour, I2C_SLAVE, SLAVE_COLOUR_ADDR) < 0) {
		printf("failed to configure the file for I2C COLOR communication.");
		exit(1);
	}

	//set i2c slave address
	if (ioctl(fd_colour, I2C_SLAVE, SLAVE_COLOUR_ADDR) < 0) {
		perror("Failed to set the I2C slave COLOR address");
		exit(1);
	}

	//init both sensors
	init_accelerometer(fd_acc);
	init_colour_sensor(fd_colour);

	printf("Getting data...\n");

	while (1) {
		for (int i = 0; i < 10; i++) {
			printf("Measure number: %d completed!\n", i+1);
			//we read data from both sensors
			read_reg(SLAVE_ACC_ADDR, fd_acc, 0x3B, acc_value, 6);
			read_reg(SLAVE_COLOUR_ADDR, fd_colour, 0x96, colour_data, 6);

			message[i].x = acc_value[0] << 8 | acc_value[1];
			message[i].y = acc_value[2] << 8 | acc_value[3];
			message[i].z = acc_value[4] << 8 | acc_value[5];

			message[i].red = colour_data[1] << 8 | colour_data[0];
			message[i].green = colour_data[3] << 8 | colour_data[2];
			message[i].blue = colour_data[4] << 8 | colour_data[4];
			sleep(1);
		}
		printf("Sending measure %d...\n", numero);
		bzero(buffer, 256);
		//fgets(buffer,255,stdin);
		memcpy(buffer, message, sizeof(message));
		n = sendto(sock, buffer, sizeof(buffer), 0,
				(const struct sockaddr*) &server, length);
		if (n < 0)
			error("Sendto");
		n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*) &from,
				&length); //256 en vez de sizeofbuffer
		if (n < 0)
			error("recvfrom");
		write(1, "Got an ack: \n", 12);
		write(1, buffer, n);
		numero = (numero == 6) ? 1: numero+1;
	}

	close(fd_colour);
	close (fd_acc);
}

void error(const char *msg) {
	perror(msg);
	exit(0);
}

//funcion que se encarga de escribir un valor concreto en una direccion de un registro
void write_reg(int slave_addr, int fd_i2c, unsigned char reg_dir,
		unsigned char write_data) {
	struct i2c_rdwr_ioctl_data packets_i2c;
	struct i2c_msg messages[1];
	int err = 0;
	unsigned char write_sequence[2];

	write_sequence[0] = reg_dir;
	write_sequence[1] = write_data;

	messages[0].addr = slave_addr;		//slave address
	messages[0].flags = 0; 					//write operation
	messages[0].len = 2;				//1 - direccion del registro, 2 - valor
	messages[0].buf = write_sequence; //Pointer to the data bytes to be written

	//configuracion de los paquetes que se mandan
	packets_i2c.msgs = messages;
	packets_i2c.nmsgs = 1;

	//Send the I2C transaction
	err = ioctl(fd_i2c, I2C_RDWR, &packets_i2c);
	if (err < 0) {
		printf("failed to configure the file for I2C communication.");
		exit(1);
	}
}

//funcion que se encarga de leer r_len bytes en una direccion de un registro reg_dir y los guarda en *read_data
void read_reg(int slave_addr, int fd, unsigned char reg_dir,
		unsigned char *read_data, int r_len) {
	struct i2c_rdwr_ioctl_data packets_i2c;
	struct i2c_msg messages[2];
	int err = 0;

	messages[0].addr = slave_addr;			//slave address
	messages[0].flags = 0;    					// Write operation
	messages[0].len = 1;    					// One byte to write
	messages[0].buf = &reg_dir;		//direccion del registro donde queremos leer

	messages[1].addr = slave_addr; 		//slave address
	messages[1].flags = I2C_M_RD; 				// Read operation
	messages[1].len = r_len;    				// Number of bytes to read
	messages[1].buf = read_data;//direccion donde queremos guardar los datos leidos

	packets_i2c.msgs = messages;
	packets_i2c.nmsgs = 2;

	err = ioctl(fd, I2C_RDWR, &packets_i2c);
	if (err < 0) {
		printf("failed to configure the file for I2C communication.");
		exit(1);
	}
}

//funcion que se encarga de iniciar el acelerómetro
void init_accelerometer(int fd) {
	write_reg(SLAVE_ACC_ADDR, fd, 0x6b, 0x80);     // Initial reset
	sleep(1); // Wait to stabilize;
	write_reg(SLAVE_ACC_ADDR, fd, 0x1C, 0x00); //set the sensibility to 2g (human movements)
	sleep(1);
	write_reg(SLAVE_ACC_ADDR, fd, 0x6c, 0xC7); //frequency of wake up 40hz, giroscope always standby mode
	sleep(1); // Wait to stabilize;
	write_reg(SLAVE_ACC_ADDR, fd, 0x6b, 0x28);//cycle active, temp disable, 8Mhz oscillator
	sleep(1); // Wait to stabilize;
	write_reg(SLAVE_ACC_ADDR, fd, 0x1a, 0x03); //brandwith 44hz delay 4.9ms
	sleep(3);
}

//funcion que se encarga de iniciar el sensor de color
void init_colour_sensor(int fd) {
	write_reg(SLAVE_COLOUR_ADDR, fd, 0x80, 0x03); //activates Power on and RGBC
	usleep(1000);
	write_reg(SLAVE_COLOUR_ADDR, fd, 0x81, 0x00); //max counts of 65535 every 700ms
	usleep(500);
	write_reg(SLAVE_COLOUR_ADDR, fd, 0x83, 0x07); //RGBC clear channer high threshold upper byte
	usleep(500);
	write_reg(SLAVE_COLOUR_ADDR, fd, 0x8F, 0x00); //x1 gain
	usleep(500);
	//write_reg(fd, 0x80, 0x43);                 // Apaga el flash
	//sleep(1);
}
