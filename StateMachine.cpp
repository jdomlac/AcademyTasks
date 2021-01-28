/* 
The following code implements the "Autonomous System state machine" states and transitions
as described on the "Formula Student Rules 2020" provided by Formula Student Germany.

Source available here: https://www.formulastudent.de/fileadmin/user_upload/all/2020/rules/FS-Rules_2020_V1.0.pdf

Following the task indications, the actual update of the states (Like updating ASMS of if RES is triggered) is considered already coded.
*/

/*
For making the code more human readable, we will use some #define, for the varius states and flags that
are represented as numbers on the code. It will be harder to understand and follow the code without this.
*/
#define AS_OFF 0
#define AS_READY 1
#define AS_DRIVING 2
#define AS_EMERGENCY 3
#define AS_FINISHED 4
#define MANUAL_DRIVE 5

#define statesNum 6

#define ASSI_OFF 0
#define ASSI_YELLOWFLASH 1
#define ASSI_YELLOW 2
#define ASSI_BLUEFLASH 3
#define ASSI_BLUE 4

#define TS_OFF 0
#define TS_ON 1

#define R2D_OFF 0
#define R2D_ON 1

#define SA_UNAVAILABLE 0
#define SA_AVAILABLE 1

#define SB_UNAVAILABLE -1
#define SB_ENGAGED 0
#define SB_AVAILABLE 1

#define EBS_UNAVAILABLE -1
#define EBS_ARMED 0
#define EBS_ACTIVATED 1

#define BRAKES_RELEASED 0
#define BRAKES_PRESSED 1

#define DONT_CARE -2

/*
The following matrix is what is known as the accesibility matrix of a directed graph, 
which represents the "Figure 21" as a graph. (See rules named at the header of the file)

When consulting this matrix this should be done like: [currState][desiredState] the returned
value will be 1 for accesible state and 0 for those who are not accesible. More info on the report of the task.
*/
const bool stateMatrix[statesNum][statesNum]{
                 {1,1,0,0,0,1},
                 {1,1,1,1,0,0},
                 {0,0,1,1,1,0},
                 {1,0,0,1,0,0},
                 {1,0,0,1,1,0},
                 {1,0,0,0,0,1}
};

/*
The following matrix contains the states of the diferent components, on a given state of the autonomous system.

stateMachineMatrix["desired state here"][i]

i will define what are you consulting, they are saved in the following order:
0 -> TS; 1 -> R2D; 2 -> SA; 3 -> SB; 4 -> EBS; 5 -> ASSI;
*/
const short stateMachineMatrix[statesNum][6]{
    {TS_OFF,    R2D_OFF,    SA_UNAVAILABLE, SB_UNAVAILABLE, DONT_CARE,      ASSI_OFF},          //AS_OFF
    {TS_ON,     R2D_OFF,    SA_AVAILABLE,   SB_ENGAGED,     EBS_ARMED,      ASSI_YELLOW},       //AS_READY
    {TS_ON,     R2D_ON,     SA_AVAILABLE,   SB_AVAILABLE,   EBS_ARMED,      ASSI_YELLOWFLASH},  //AS_DRIVING
    {TS_OFF,    R2D_OFF,    DONT_CARE,      DONT_CARE,      EBS_ACTIVATED,  ASSI_BLUEFLASH},    //AS_EMERGENCY
    {TS_OFF,    R2D_OFF,    SA_UNAVAILABLE, DONT_CARE,      EBS_ACTIVATED,  ASSI_BLUE},         //AS_FINISHED
    {TS_ON,     R2D_ON,     SA_UNAVAILABLE, SB_UNAVAILABLE, EBS_UNAVAILABLE,ASSI_OFF}           //MANUAL_DRIVING
};


//The remaining vars define the state of the diferent components of the car, as some external flags like the ERS or Go Signal
int currState = AS_OFF;     //Initial state must be AS_OFF
int ASSIState = ASSI_OFF;   //Initial state must be ASSI_OFF
bool on = true;             //This variable is used for the main loop, when changed to false, the loop, and the whole program will stop

//Defining the initial states, as stated on "Figure 21" (See rules named at the header of the file)
bool TS = TS_OFF;               //Initial state of the tractive system must be off
bool R2D = R2D_OFF;             //Initial state of the "Ready to drive" must be off
bool SA = SA_UNAVAILABLE;       //Initial state of the steering actuation must be unavailable
short SB = SB_UNAVAILABLE;      //Initial state of the service brake must be unavailable
short EBS = EBS_UNAVAILABLE;    //Actually, the initial state of the EBS doesn't care

//OtherFlags (This aren't updated by this code in any moment)
bool goSignal = false;
bool AutonomousMissionSelected = false;
bool ManualMissionSelected = false;
bool MissionFinished = false;
bool ASMS = false;
bool Brakes = BRAKES_PRESSED;
bool RESTriggered = false;
bool EBSSound = false;
int delay = 0;  //This var stores the value (in secs) of time elapsed after engaging AS_READY
double speed = 0;

bool updateState(int desiredState){
    TS = stateMachineMatrix[desiredState][0];
    R2D = stateMachineMatrix[desiredState][1];
    if(stateMachineMatrix[desiredState][2] != DONT_CARE) { SA = stateMachineMatrix[desiredState][2]; }
    if(stateMachineMatrix[desiredState][3] != DONT_CARE) { SB = stateMachineMatrix[desiredState][3]; }
    if(stateMachineMatrix[desiredState][4] != DONT_CARE) { EBS = stateMachineMatrix[desiredState][4]; }

    return true;
}

/*
The update of the ASSIs has been coded separately in this function
This has been done to ensure that is clear that the ASSIs are being updated before the transition is completed, to be complying with DV 2.4.3
*/
void updateASSI(){
    ASSIState = stateMachineMatrix[currState][5];
}

bool stateMachine(int desiredState){

    //If the state given is not a valid one, or we are already on that same state, the function will stop
    if(desiredState <= 0 && desiredState < statesNum && currState == desiredState) { return false; }
    
    /*
    If the desired state is not accesible from our current state the function will stop, this should never happen
    as when we are in one state we are only checking the accessible ones, but it is always important to check for redundancy
    */
    if(stateMatrix[currState][desiredState] != true) { return false; }

    /*
    Proceed with the actual state update, if something goes wrong (on the updateState function), the code will return false
    and won't continue, meaning that ASSIs will not update, this is done in purpose, as we cannot show a state on the ASSIs
    if the transition hasn't been completed.
    This is not the case in this code as we are just updating some vars, but it is ready to work in the event of an supposed failure
    */
    if(!updateState(desiredState)) { return false; }

    /*
    When finished updating state, we proceed to update our var currState and update the ASSIs
    This is done after fully updating state to comply with DV 2.4.3
    "Until the transition is complete the ASSIs must indicate the initial state."
    */
    currState = desiredState;
    updateASSI();
    return true;
}

/*
Main loop of the code, this can be shut down by changing the "on" var to false

The purpose of this piece of code is to first, check what state are we in, an then the posible new states
and if the conditions to change are met, then request the change.
*/
void main() {
    do{
        switch(currState){

        case AS_OFF:
            if(AutonomousMissionSelected && EBS == EBS_ARMED && ASMS && TS == TS_ON) { stateMachine(AS_READY); }
            if(ManualMissionSelected && EBS == EBS_UNAVAILABLE && !ASMS && TS == TS_ON) { stateMachine(MANUAL_DRIVE); }
            break;
        
        case AS_READY:
            if(!ASMS && Brakes == BRAKES_RELEASED) { stateMachine(AS_OFF); }
            if(EBS == EBS_ACTIVATED) { stateMachine(AS_EMERGENCY); }
            if(goSignal && (delay >= 5)) { stateMachine(AS_DRIVING); }
            break;

        case AS_DRIVING:
            if(EBS == EBS_ACTIVATED) { stateMachine(AS_EMERGENCY); }
            if(MissionFinished && speed == 0) { stateMachine(AS_FINISHED); }
            break;
        
        case AS_EMERGENCY:
            if(!EBSSound && !ASMS && Brakes == BRAKES_RELEASED){ stateMachine(AS_OFF); }
            break;

        case AS_FINISHED:
            if(RESTriggered) { stateMachine(AS_EMERGENCY); }
            if(!ASMS && Brakes == BRAKES_RELEASED) { stateMachine(AS_OFF); }
            break;
        
        case MANUAL_DRIVE:
            if(TS == TS_OFF) { stateMachine(AS_OFF); }
            break;
        }
    } while(on);
}