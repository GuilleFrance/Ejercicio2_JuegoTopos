#include "mbed.h"
#include "stdlib.h"

#define TRUE            1
#define FALSE           0
#define NROBOTONES      4
#define MAXLED          4
#define TIMETOSTART     1000
#define BASETIME        500
#define TIMEMAX         1501

#define ESPERAR         0
#define JUEGO           1
#define FINDELJUEGO     2
#define TECLAS          3
#define HBTIME          200
#define NUMBEAT         4

/**
 * @brief Defino el intervalo entre lecturas para "filtrar" el ruido del Botón
 */
#define INTERVAL        40

/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,    //0
    BUTTON_UP,      //1
    BUTTON_FALLING, //2
    BUTTON_RISING   //3
}_eButtonState;

typedef struct{
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;

_sTeclas ourButton[NROBOTONES];

uint16_t mask[]={0x0001,0x0002,0x0004,0x0008};
uint8_t estadoJuego=ESPERAR;

/**
 * @brief Dato del tipo Enum para asignar los estados de la MEF
 */
_eButtonState myButton;

/**
 * @brief Inicializa la MEF
 * Le dá un estado inicial a myButton
 * 
 */
void startMEF(uint8_t indice);

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 */
void refreshMEF(uint8_t indice );

/**
 * @brief Función para cambiar el estado del LED cada vez que sea llamada.
 * 
 */
void toggleLed(uint8_t indice);


/**
 * @brief Latido!
 * Destella el led de la placa para comprobar que el programa funciona
 * 
 */
void heartbeat(void);

BusIn pulsadores(PB_6,PB_7,PB_8,PB_9); //!< Entrada del grupo de pulsadores que funcionaran como "martillos"
BusOut leds(PB_12,PB_13,PB_14,PB_15); //!< Salida del grupo de leds que cumpliran el rol de "topos"

DigitalOut heartBeat(PC_13); //!< Led que voy a usar para que realize el heartbeat
Timer miTimer; //!< Timer para hacer la espera de 40 milisegundos del control de la Mef
int tiempoMs=0; //!< variable donde voy a almacenar el tiempo del timmer una vez cumplido

int main(){
    //Testeo de que andan los leds
    leds=15;
    wait_ms (1000);
    leds=0;

    miTimer.start();
    uint16_t ledAuxRandom=0;
    int tiempoRandom=0;
    int ledAuxRandomTime=0;
    int ledAuxJuegoStart=0;
    int auxBotonesPresionados=0;
    int auxBandera=0;
    int win=0;
    int countSecuencia=0, tiempoSecuencias=0;
    uint8_t indiceAux=0;
    int beatTime=0;
    heartBeat=1;
    
    for(uint8_t indice=0; indice<NROBOTONES;indice++){
        startMEF(indice);
    }

    while(TRUE)
    {
        /// Llamado a la funcion heartbeat
        if ((miTimer.read_ms()-beatTime)>HBTIME){
           beatTime=miTimer.read_ms();
           heartbeat();
        }

        switch(estadoJuego){
            case ESPERAR:
                if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
                    tiempoMs=miTimer.read_ms();
                    for(uint8_t indice=0; indice<NROBOTONES;indice++){
                        refreshMEF(indice);
                        if(ourButton[indice].timeDiff >= TIMETOSTART){
                            srand(miTimer.read_us());
                            estadoJuego=TECLAS;
                        }
                    }
                }
            break;
            case TECLAS:
                for(indiceAux=0; indiceAux<NROBOTONES;indiceAux++){
                    refreshMEF(indiceAux);
                    if(ourButton[indiceAux].estado!=BUTTON_UP){
                        break;
                    }
                }
                if(indiceAux==NROBOTONES){
                    estadoJuego=JUEGO;
                    leds=15;
                    ledAuxJuegoStart=miTimer.read_ms();
                }
            break;
            case JUEGO:
                if(leds==0){
                    ledAuxRandom = rand() % (MAXLED);
                    ledAuxRandomTime = (rand() % (TIMEMAX))+BASETIME;
                    tiempoRandom=miTimer.read_ms();
                    toggleLed(ledAuxRandom);    // leds=mask[ledAuxRandom];
                }else{
                    if((miTimer.read_ms() - ledAuxJuegoStart)> TIMETOSTART) {
                        if(leds==15){
                            ledAuxJuegoStart=miTimer.read_ms();
                            leds=0;
                        }
                    }
                }
                
                if((miTimer.read_ms()-tiempoRandom)<=ledAuxRandomTime){
                    for(indiceAux=0; indiceAux<NROBOTONES;indiceAux++){
                        refreshMEF(indiceAux);
                        if(ourButton[indiceAux].estado!=BUTTON_UP){
                            auxBotonesPresionados=(indiceAux+1);
                            auxBandera+=(indiceAux+1);
                        }
                    }
                            if(auxBotonesPresionados!=auxBandera){
                                //Perdí (apreté más de un botón)
                                win=0;
                            }else{
                                //Apreté solo un botón
                                if((auxBotonesPresionados-1)==ledAuxRandom){
                                    //Gané
                                    win=1;
                                    estadoJuego=FINDELJUEGO;
                                }else{
                                    //Perdí (Apreté el botón equivocado)
                                    win=0;
                                    estadoJuego=FINDELJUEGO;
                                }
                            }
                            leds=0;
                            auxBotonesPresionados=0;
                }
                
                if((miTimer.read_ms()-tiempoRandom)>ledAuxRandomTime){
                    //estadoJuego=FINDELJUEGO;
                    leds=0;
                    wait_ms(1000);
                    win=0;
                    ledAuxRandom=3;
                    estadoJuego=FINDELJUEGO;
                }
            break;
            case FINDELJUEGO:
                if(win==1){
                    //Secuencia para cuando ganas
                    if((miTimer.read_ms() - tiempoSecuencias)> BASETIME) {
                        tiempoSecuencias=miTimer.read_ms();
                        if(leds==0){
                            leds=15;
                        }else{
                            leds=0;
                        }
                        countSecuencia++;
                    }
                }else{
                    //Secuencia para cuando perdes
                    if((miTimer.read_ms() - tiempoSecuencias)> BASETIME) {
                        tiempoSecuencias=miTimer.read_ms();
                        if(leds==0){
                            toggleLed(ledAuxRandom);
                        }else{
                            leds=0;
                        }
                        countSecuencia++;
                    }
                }
                if(countSecuencia>5){
                    countSecuencia=0;
                    win=0;
                    estadoJuego=ESPERAR;
                }
            break;
            default:
                estadoJuego=ESPERAR;
        }
    }
    return 0;
}

void startMEF(uint8_t indice){
   ourButton[indice].estado=BUTTON_UP;
}

void refreshMEF(uint8_t indice){

    switch (ourButton[indice].estado)
    {
    case BUTTON_DOWN:
        if(pulsadores.read() & mask[indice] )
           ourButton[indice].estado=BUTTON_RISING;
    
    break;
    case BUTTON_UP:
        if(!(pulsadores.read() & mask[indice]))
            ourButton[indice].estado=BUTTON_FALLING;
    
    break;
    case BUTTON_FALLING:
        if(!(pulsadores.read() & mask[indice]))
        {
            ourButton[indice].timeDown=miTimer.read_ms();
            ourButton[indice].estado=BUTTON_DOWN;
            //Flanco de bajada
        }
        else
            ourButton[indice].estado=BUTTON_UP;    

    break;
    case BUTTON_RISING:
        if(pulsadores.read() & mask[indice]){
            ourButton[indice].estado=BUTTON_UP;
            //Flanco de Subida
            ourButton[indice].timeDiff=miTimer.read_ms()-ourButton[indice].timeDown;
        }else
            ourButton[indice].estado=BUTTON_DOWN;
    break;
    
    default:
        startMEF(indice);
    break;
    }
}

void toggleLed(uint8_t indice){
   leds=mask[indice];
}

void heartbeat(void){
    static uint8_t index=0;
    if(index<NUMBEAT){
        heartBeat=!heartBeat;
        index++;
    }else{
        heartBeat=1;
        index = (index>=25) ? (0) : (index+1);   
    }
}