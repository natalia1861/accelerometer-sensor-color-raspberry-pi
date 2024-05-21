/* Creates a datagram server.  The port
   number is passed as an argument.  This
   server runs forever */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <math.h>

typedef struct {
	float x;
	float y;
	float z;
	float red;
	float green;
	float blue;
} client_msg;

// Estructura para almacenar las estadísticas de cada miembro
typedef struct {
    client_msg mensaje_max;
    client_msg mensaje_min;
    client_msg mensaje_mean;
    client_msg mensaje_std;
} stats;

void mostrar_resultados(client_msg mensaje, int threshold);
void calcularEstadisticas(client_msg *valores, int n, stats *estadisticas);

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
	//net variables
   int sock, length, n;
   socklen_t fromlen;
   struct sockaddr_in server;
   struct sockaddr_in from;
   int n_msg = 0;

   //sensor variables
   int umbral_color = 15000;
   client_msg rec_msg[60];
   client_msg *ptr = rec_msg;
   stats valores_estadisticas;

   if (argc < 2) {
      fprintf(stderr, "ERROR, no port provided\n");
      exit(0);
   }

	//net code (copied)
   sock=socket(AF_INET, SOCK_DGRAM, 0);
   if (sock < 0) error("Opening socket");
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=INADDR_ANY;
   server.sin_port=htons(atoi(argv[1]));
   if (bind(sock,(struct sockaddr *)&server,length)<0)
       error("binding");
   fromlen = sizeof(struct sockaddr_in);
   while (1) {
       n = recvfrom(sock,rec_msg+10*n_msg,sizeof(client_msg[10]),0,(struct sockaddr *)&from,&fromlen); //antes ponia 1024 en vez de sizeofbuf
       if (n < 0) error("recvfrom");
       printf ("\n\nRESULTADO NUMERO: %d\n", (n_msg+1));
       for (int i = 0; i< 10; i++) {
    	   printf("  Medicion numero: %d \n", i+1);
    	   mostrar_resultados(rec_msg[i+10*n_msg], umbral_color);	//we print results
       }
       n = sendto(sock,"Got your message\n",17,
                  0,(struct sockaddr *)&from,fromlen);
       if (n  < 0) error("sendto");

       if (n_msg == 5) {	//if we completed all data measurements (1 min) we send it
    	   printf ("\n\nCALCULANDO VALORES ESTADISTICOS...\n");
    	   calcularEstadisticas(rec_msg, 60, &valores_estadisticas);
    	   printf ("   Valores promedios: \n");
    	   mostrar_resultados(valores_estadisticas.mensaje_mean, umbral_color);
    	   printf ("   Valores máximos: \n");
    	   mostrar_resultados(valores_estadisticas.mensaje_max, umbral_color);
    	   printf ("   Valores mínimos: \n");
    	   mostrar_resultados(valores_estadisticas.mensaje_min, umbral_color);
    	   printf ("   Valores desviación típica: \n");
    	   mostrar_resultados(valores_estadisticas.mensaje_std, umbral_color);
    	   n_msg = 0;
       } else {
    	   n_msg++;
       }
   }
   return 0;
 }

//print results 
void mostrar_resultados(client_msg mensaje, int threshold) {
	//Print values and sleep 3 seconds
	printf("    X-axis: %f, Y-axis: %f, Z-axis: %f\n", mensaje.x, mensaje.y,
			mensaje.z);
	//Print values and sleep 3 seconds
	printf("    \x1b[31mRED: \x1b[0m %f, \x1b[32m GREEN: \x1b[0m%f, \x1b[36m BLUE: \x1b[0m %f\n",
			mensaje.red, mensaje.green, mensaje.blue);
	// Determinar el color predominante basado en los umbrales
	if (mensaje.red > mensaje.green && mensaje.red > mensaje.blue
			&& mensaje.red > threshold) {
		printf("      El color predominante es: \x1b[31m Rojo \x1b[0m\n");
	} else if (mensaje.green > mensaje.red && mensaje.green > mensaje.blue
			&& mensaje.green > threshold) {
		printf("      El color predominante es: \x1b[32m Verde\x1b[0m\n");
	} else if (mensaje.blue > mensaje.red && mensaje.blue > mensaje.green
			&& mensaje.blue > threshold) {
		printf("      El color predominante es: \x1b[36m Azul\x1b[0m\n");
	} else if (mensaje.red > threshold && mensaje.green > threshold
			&& mensaje.blue > threshold) {
		printf("      El color predominante es: \x1b[37m Blanco\x1b[0m\n");
	} else {
		printf("      El color predominante es: \x1b[40m\x1b[37mNegro\x1b[0m\n");
	}
}

// Función para calcular las estadísticas para cada miembro de client_msg y almacenarlas en stats
void calcularEstadisticas(client_msg *valores, int n, stats *estadisticas) {
	// Inicializar los valores de las estadísticas con el primer elemento
	estadisticas->mensaje_max = valores[0];
	estadisticas->mensaje_min = valores[0];
	estadisticas->mensaje_mean = valores[0];
	estadisticas->mensaje_std = valores[0];

	// Calcula la suma para cada miembro
	for (int i = 0; i < n; i++) {
		estadisticas->mensaje_mean.x += valores[i].x;
		estadisticas->mensaje_mean.y += valores[i].y;
		estadisticas->mensaje_mean.z += valores[i].z;
		estadisticas->mensaje_mean.red += valores[i].red;
		estadisticas->mensaje_mean.green += valores[i].green;
		estadisticas->mensaje_mean.blue += valores[i].blue;

		// Calcula el máximo y el mínimo para cada miembro
		if (valores[i].x > estadisticas->mensaje_max.x)
			estadisticas->mensaje_max.x = valores[i].x;
		if (valores[i].y > estadisticas->mensaje_max.y)
			estadisticas->mensaje_max.y = valores[i].y;
		if (valores[i].z > estadisticas->mensaje_max.z)
			estadisticas->mensaje_max.z = valores[i].z;
		if (valores[i].red > estadisticas->mensaje_max.red)
			estadisticas->mensaje_max.red = valores[i].red;
		if (valores[i].green > estadisticas->mensaje_max.green)
			estadisticas->mensaje_max.green = valores[i].green;
		if (valores[i].blue > estadisticas->mensaje_max.blue)
			estadisticas->mensaje_max.blue = valores[i].blue;

		if (valores[i].x < estadisticas->mensaje_min.x)
			estadisticas->mensaje_min.x = valores[i].x;
		if (valores[i].y < estadisticas->mensaje_min.y)
			estadisticas->mensaje_min.y = valores[i].y;
		if (valores[i].z < estadisticas->mensaje_min.z)
			estadisticas->mensaje_min.z = valores[i].z;
		if (valores[i].red < estadisticas->mensaje_min.red)
			estadisticas->mensaje_min.red = valores[i].red;
		if (valores[i].green < estadisticas->mensaje_min.green)
			estadisticas->mensaje_min.green = valores[i].green;
		if (valores[i].blue < estadisticas->mensaje_min.blue)
			estadisticas->mensaje_min.blue = valores[i].blue;
	}

	// Calcula el promedio para cada miembro
	estadisticas->mensaje_mean.x /= n;
	estadisticas->mensaje_mean.y /= n;
	estadisticas->mensaje_mean.z /= n;
	estadisticas->mensaje_mean.red /= n;
	estadisticas->mensaje_mean.green /= n;
	estadisticas->mensaje_mean.blue /= n;

	// Calcula la desviación estándar para cada miembro
	for (int i = 0; i < n; i++) {
		estadisticas->mensaje_std.x += pow(
				valores[i].x - estadisticas->mensaje_mean.x, 2);
		estadisticas->mensaje_std.y += pow(
				valores[i].y - estadisticas->mensaje_mean.y, 2);
		estadisticas->mensaje_std.z += pow(
				valores[i].z - estadisticas->mensaje_mean.z, 2);
		estadisticas->mensaje_std.red += pow(
				valores[i].red - estadisticas->mensaje_mean.red, 2);
		estadisticas->mensaje_std.green += pow(
				valores[i].green - estadisticas->mensaje_mean.green, 2);
		estadisticas->mensaje_std.blue += pow(
				valores[i].blue - estadisticas->mensaje_mean.blue, 2);
	}
    estadisticas->mensaje_std.x = sqrt((double)estadisticas->mensaje_std.x / n);
    estadisticas->mensaje_std.y = sqrt((double)estadisticas->mensaje_std.y / n);
    estadisticas->mensaje_std.z = sqrt((double)estadisticas->mensaje_std.z / n);
    estadisticas->mensaje_std.red = sqrt((double)estadisticas->mensaje_std.red / n);
    estadisticas->mensaje_std.green = sqrt((double)estadisticas->mensaje_std.green / n);
    estadisticas->mensaje_std.blue = sqrt((double)estadisticas->mensaje_std.blue / n);
}



