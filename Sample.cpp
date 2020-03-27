// Ecole Centrale de Nantes
// Sebastian CEPEDA ESPINOSA
// Projet d'Option Sciences et Musique
// V1.0
//------------------------
// Description :
//  Shyntese du son de Theremin et commande de ces parametres: Frequence et Amplitude
//  Par rapport aux lectures du tracking des mains fait avec du LeapMotion
// ------------------------

#include <iostream>
#include <cstring>
#include <math.h>
#include "Leap.h" 
//This for the synthe
#include <sys/ioctl.h> //for ioctl()
#include <math.h> //sin(), floor(), and pow()
#include <stdio.h> //perror
#include <fcntl.h> //open, O_WRONLY
#include <linux/soundcard.h> //SOUND_PCM*
#include <unistd.h> // Librairie employee dans la generation du son

using namespace std;
 
#define TYPE char // car son de 8 bits
#define LENGTH 0.1 //number of seconds per frequency
#define RATE 40960 //sampling rate
#define SIZE sizeof(TYPE) //size of sample, in bytes
#define CHANNELS 1 //number of audio channels
#define PI 3.14159
#define NUM_FREQS 1 //total number of frequencies
#define BUFFSIZE (int) (NUM_FREQS*LENGTH*RATE*SIZE*CHANNELS) //bytes sent to audio device
#define ARRAYSIZE (int) (NUM_FREQS*LENGTH*RATE*CHANNELS) //total number of samples
#define SAMPLE_MAX (pow(2,SIZE*8 - 1) - 1) 
#define TABLESAMPLES 40960 // Taille du tableau de basse pour reechantillonage
#define TABLESIZE (int) (TABLESAMPLES*SIZE)
//Define des gains pour le control de pitch:
#define WLX (float) ((1/600.0f)*3*(2000/5))
#define WLY (float) ((1/500.0f)*1*(2000/5))
#define WLZ (float) ((1/300.0f)*1*(2000/5))
//Define des gains pour le control d'amplitude:
#define WRY (float) ((1/400.0f)*5*(128/5))


using namespace Leap;

    
//Definir buffer pour la transmision des données et tableau pour échantilloner
TYPE buf[ARRAYSIZE];
TYPE table[TABLESIZE];

int f = 440;
int ampl = 128;

// SampleListener Heritage de la classe Listener de l'API LeapMotion
class SampleListener : public Listener {
  public:
    virtual void onInit(const Controller&);
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onExit(const Controller&);
    virtual void onFrame(const Controller&);
    virtual void onFocusGained(const Controller&);
    virtual void onFocusLost(const Controller&);
    virtual void onDeviceChange(const Controller&);
    virtual void onServiceConnect(const Controller&);
    virtual void onServiceDisconnect(const Controller&);

  private:
};

// necessaires pour le calcul de la position
const std::string fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
const std::string boneNames[] = {"Metacarpal", "Proximal", "Middle", "Distal"};
const std::string stateNames[] = {"STATE_INVALID", "STATE_START", "STATE_UPDATE", "STATE_END"};

// Methodes actives au momment de detection de l'evenement
void SampleListener::onInit(const Controller& controller) {
  std::cout << "Initialized" << std::endl;
}

void SampleListener::onConnect(const Controller& controller) {
  std::cout << "Connected" << std::endl;
  controller.enableGesture(Gesture::TYPE_CIRCLE);
  controller.enableGesture(Gesture::TYPE_KEY_TAP);
  controller.enableGesture(Gesture::TYPE_SCREEN_TAP);
  controller.enableGesture(Gesture::TYPE_SWIPE);
}

void SampleListener::onDisconnect(const Controller& controller) {
  // Note: not dispatched when running in a debugger.
  std::cout << "Disconnected" << std::endl;
}

void SampleListener::onExit(const Controller& controller) {
  std::cout << "Exited" << std::endl;
}


//--------------------My fonction¡------------

void SampleListener::onFrame(const Controller& controller) {
   const Frame frame = controller.frame(); // prends la lecture des capteurs
   HandList hands = frame.hands(); // Extrait information des mains
   // Execute une sousrutines pour chaque main
   for (HandList::const_iterator hl = hands.begin(); hl != hands.end(); ++hl) {
     const Hand hand = *hl;
     if(hand.isLeft()){ // main gauche: calcule de frequence
        Leap::Vector left = hand.palmPosition();
        //------------------
        //X left[0]; -300 to 300
        //Y left[1]; 0 to 500
        //Z left[2]; -300 to 300
        //------------------
        f = abs((int)(WLX*abs((int)(-300+left[0]))+WLY*abs((int)(left[1]))+WLZ*abs((int)(left[2]))));
     }
     if(hand.isRight()){// main droite: calcule de amplitude
        Leap::Vector right = hand.palmPosition();
        //------------------
        //X right[0]; -300 to 300
        //Y right[1]; 0 to 500
        //Z right[2]; -300 to 300
        //------------------
        int ampl_temp = WRY*abs((int)(right[1]));
        
        // saturation de l'amplitude: voir rapport
        if(ampl_temp>128){
         ampl=127;
        }else{
            if(ampl_temp<0){
                ampl=0; 
            }else{
                ampl=ampl_temp;
            }
        }
     }
   }
   // Decomenter pour voir la frequence et la amplitude en temps reel
   //std::cout<<"Frequence: "<<f<<" Amplitude "<<ampl<<std::endl;
}
//-------------Fonctions necesaires pour le fonctionement de la connection avec Leap Motion
void SampleListener::onFocusGained(const Controller& controller) {
  std::cout << "Focus Gained" << std::endl;
}

void SampleListener::onFocusLost(const Controller& controller) {
  std::cout << "Focus Lost" << std::endl;
}

void SampleListener::onDeviceChange(const Controller& controller) {
  std::cout << "Device Changed" << std::endl;
  const DeviceList devices = controller.devices();

  for (int i = 0; i < devices.count(); ++i) {
    std::cout << "id: " << devices[i].toString() << std::endl;
    std::cout << "  isStreaming: " << (devices[i].isStreaming() ? "true" : "false") << std::endl;
  }
}

void SampleListener::onServiceConnect(const Controller& controller) {
  std::cout << "Service Connected" << std::endl;
}

void SampleListener::onServiceDisconnect(const Controller& controller) {
  std::cout << "Service Disconnected" << std::endl;
}

//-----------Fonctions auxiliers du Synthètiseur------------


char constrain(float number, int max, int min){
    // Function trditionel de saturation qui transforme aussi de float à uint_8t
    if(number>max)
        return max;
    if(number<min)
        return min;
    
    return (int)number;
}

int main(int argc, char** argv) {

    int deviceID, arg, status, t, a, i;

    // Generer le tableau de basse pour sous-échantilloner avec les caracteristiques décris:
    int brightness = 128;
    int waveform = 128;
    float br = (1.0f + 3.0f*((float)(brightness)/(float)255.0))*((float)6.0/PI);
    float wf = 0.8f * (float)(waveform/255.0f);
    int dacMax = ampl -1;
    int dacMin = 0- ampl;
    float hp = PI * 2.0f / (float)TABLESAMPLES;
    
    for(t = 0; t< TABLESIZE; t++){
          table[t] = constrain(ampl * tanh((asin(sin(t*hp)) + wf)*br),dacMax,dacMin);
    }
    
    //Demarrer la transmition d'information pour la carte de son
    deviceID = open("/dev/dsp", O_WRONLY, 0);
      if (deviceID < 0)
            perror("Opening /dev/dsp failed\n");
    // working
      arg = SIZE * 8;
      status = ioctl(deviceID, SNDCTL_DSP_SETFMT, &arg);
      if (status == -1)
            perror("Unable to set sample size\n");
      arg = CHANNELS;
      status = ioctl(deviceID, SNDCTL_DSP_CHANNELS, &arg);
      if (status == -1)
            perror("Unable to set number of channels\n");
      arg = RATE;
      status = ioctl(deviceID, SNDCTL_DSP_SPEED, &arg);
      if (status == -1)
            perror("Unable to set sampling rate\n");
    
    
  // Create a sample listener and controller
  SampleListener listener;
  Controller controller;

  // Have the sample listener receive events from the controller
  controller.addListener(listener);
  
  while(true){
    for (t = 0, i=0; t < ARRAYSIZE; ++t, i+=f) {
        if(i>=TABLESAMPLES){
            i=fmod(i,TABLESAMPLES);
        }
          buf[t]= (int)constrain(((ampl*table[i])/128.0f), 128, 0);
        }
        status = write(deviceID, buf, BUFFSIZE);
    }

  // Remove the sample listener when done
  controller.removeListener(listener);

  return 0;
}
