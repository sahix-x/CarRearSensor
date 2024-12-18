
#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include <util/delay.h>

// TODO: declare variables for cross-task communication

int sonarReading = 0;
int rawReading = 0;
bool convertBool = 0;

const int threshold_close = 8;
const int threshold_far = 12;

// TODO: Change this depending on which exercise you are doing.
// Exercise 1: 3 tasks
// Exercise 2: 5 tasks
// Exercise 3: 7 tasks
#define NUM_TASKS 5

// Task struct for concurrent synchSMs implmentations
typedef struct _task
{
    signed char state;         // Task's current state
    unsigned long period;      // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int);       // Task tick function
} task;

// TODO: Define Periods for each task
//  e.g. const unsined long TASK1_PERIOD = <PERIOD>
const int sonarPeriod = 1000;
const int displayPeriod = 1;
const int leftButtonPeriod = 200;
const int RedPeriod = 1;
const int GreenPeriod = 1;

const unsigned long GCD_PERIOD = 1;

task tasks[NUM_TASKS]; // declared task array with NUM_TASKS amount of tasks

// TODO: Define, for each task:
//  (1) enums and
//  (2) tick functions
enum Sonar_states
{
    SonarStart,
    Wait,
    Measure,
};

int Sonar_Tick(int state)
{
    // State Transitions
    switch (state)
    {
    case SonarStart:
        sonarReading = 0;
        state = Measure;
        break;

    case Wait:
        if (!GetBit(PINB, 0))
        {
            state = Measure;
        }
        else
        {
            state = Wait;
        }
        break;

    case Measure:
        state = Measure;
        break;

    default:
        state = SonarStart;
        break;
    }

    // State Actions
    switch (state)
    {
    case SonarStart:
        break;

    case Wait:
        break;

    case Measure:
        // Capture raw distance in centimeters
        rawReading = (int)round(sonar_read());
        // serial_println(rawReading, 10);

        // Convert to the desired unit based on  flag
        sonarReading = convertBool ? round(rawReading * 0.393701) : rawReading;
        // serial_println(sonarReading, 10);
        break;

    default:
        break;
    }

    return state;
}

enum Display_states
{
    DisplayStart,
    RightOn,
    RightMiddle,
    LeftMiddle,
    LeftOn,
};

int Display_Tick(int Dstate)
{
    // State Transitions
    switch (Dstate)
    {
    case DisplayStart:
        Dstate = RightOn;
        break;
    case RightOn:
        Dstate = RightMiddle;
        break;
    case RightMiddle:
        Dstate = LeftMiddle;
        break;
    case LeftMiddle:
        Dstate = LeftOn;
        break;
    case LeftOn:
        Dstate = RightOn;
        break;
    default:
        Dstate = DisplayStart;
        break;
    }
    // State Actions
    switch (Dstate)
    {
    case DisplayStart:
        break;
    case RightOn:
        PORTB = SetBit(PORTB, 5, 1); // Sets Left to hi/off
        PORTB = SetBit(PORTB, 2, 0); // Sets Right to Low/on
        outNum(sonarReading % 10);
        break;
    case RightMiddle:
        PORTB = SetBit(PORTB, 2, 1); // Sets Right to high/off
        PORTB = SetBit(PORTB, 3, 0); // sets Right Middle to low/on
        outNum((sonarReading / 10) % 10);
        break;
    case LeftMiddle:
        PORTB = SetBit(PORTB, 3, 1); // Sets Right middle to high/off
        PORTB = SetBit(PORTB, 4, 0); // Sets Left middle to low/on
        outNum((sonarReading / 100) % 10);
        break;
    case LeftOn:
        PORTB = SetBit(PORTB, 4, 1); // sets left middle to high/off
        PORTB = SetBit(PORTB, 5, 0); // sets left to high/off
        outNum((sonarReading / 1000) % 10);
        break;
    default:
        break;
    }
    return Dstate;
}

enum Button_states
{
    buttonStart,
    PressToIn,
    ReleaseToIn,
    PressToCM,
    ReleaseToCM
};

int Button_L_Tick(int LBstate)
{
    // state Transitions
    switch (LBstate)
    {
    case buttonStart:
        LBstate = PressToIn;
        break;

    case PressToIn:
        if (!GetBit(PINC, 1)) // if button is pressed
        {
            LBstate = ReleaseToIn;
        }
        else
        {
            LBstate = PressToIn;
        }
        break;

    case ReleaseToIn:
        if (GetBit(PINC, 1)) // if button is released
        {
            convertBool = 0; // Transition to inches
            LBstate = PressToCM;
        }
        else
        {
            LBstate = ReleaseToIn;
        }
        break;

    case PressToCM:
        if (!GetBit(PINC, 1)) // button is pressed again
        {
            LBstate = ReleaseToCM;
        }
        else
        {
            LBstate = PressToCM;
        }
        break;

    case ReleaseToCM:
        if (GetBit(PINC, 1)) // button is released
        {
            convertBool = 1; // Transition to centimeters
            LBstate = PressToIn;
        }
        else
        {
            LBstate = ReleaseToCM;
        }
        break;

    default:
        LBstate = buttonStart;
        break;
    }
    return LBstate;
}

enum Red_states
{
    redStart,
    redOn,
    redOff 
};
int Red_Tick(int Rstate)
{
    static unsigned char i = 0;
    static unsigned redDutyCycle = 0;

    // Duty Cycle Logic
    if (rawReading < threshold_close)
    {
        redDutyCycle = 10; // 100%
    }
    else if ((threshold_close <= rawReading) && (rawReading <= threshold_far))
    {
        redDutyCycle = 9; // 90%
    }
    else if (rawReading > threshold_far)
    {
        redDutyCycle = 0; // 0%
    }

    // State Transitions
    switch (Rstate)
    {
    case redStart:
        i = 0;
        Rstate = redOn;
        break;

    case redOn:
        if (i < redDutyCycle)
        {
            PORTC = SetBit(PORTC, 3, 1);
            i++; // Increment here when LED is on
        }
        else
        {
            Rstate = redOff;
        }
        break;

    case redOff:
        if (i < (10 - redDutyCycle))
        {
            PORTC = SetBit(PORTC, 3, 0);
            i++; // Increment here when LED is off
        }
        else
        {
            i = 0;          // Reset counter for the next cycle
            Rstate = redOn; // Switch back to redOn
        }
        break;

    default:
        Rstate = redStart; // Reset to start state
        break;
    }

    return Rstate;
}

enum Green_states
{
    greenStart,
    greenOn,
    greenOff
};

int Green_Tick(int Gstate)
{
    static unsigned char i = 0;
    static unsigned GreenDutyCycle = 0;

    // Duty Cycle Logic
    if (rawReading < threshold_close)
    {
        GreenDutyCycle = 0; // 0%
    }
    else if ((threshold_close <= rawReading) && (rawReading <= threshold_far))
    {
        GreenDutyCycle = 3; // 30%
    }
    else if (rawReading > threshold_far)
    {
        GreenDutyCycle = 10; // 100%
    }

    // State Transitions
    switch (Gstate)
    {
    case greenStart:
        i = 0;
        Gstate = greenOn;
        PORTC = SetBit(PORTC, 4, 0);
        break;

    case greenOn:
        if (i < GreenDutyCycle)
        {
            PORTC = SetBit(PORTC, 4, 1);
            i++; // Increment here when LED is on
        }
        else
        {
            Gstate = greenOff;
        }
        break;

    case greenOff:
        if (i < (10 - GreenDutyCycle))
        {
            PORTC = SetBit(PORTC, 4, 0);
            i++; // Increment here when LED is off
        }
        else
        {
            i = 0;            // Reset counter for the next cycle
            Gstate = greenOn; // Switch back to greenOn
        }
        break;

    default:
        Gstate = greenStart; // Reset to start state
        break;
    }

    return Gstate;
}

void TimerISR()
{
    // TODO: sample inputs here
    for (unsigned int i = 0; i < NUM_TASKS; i++)
    { // Iterate through each task in the task array
        if (tasks[i].elapsedTime == tasks[i].period)
        {                                                      // Check if the task is ready to tick
            tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
            tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
        }
        tasks[i].elapsedTime += GCD_PERIOD; // Increment the elapsed time by GCD_PERIOD
    }
}

int main(void)
{
    // TODO: initialize all your inputs and ouputs

    DDRB = 0xFE;
    PORTB = 0x01;

    DDRC = 0xFC;
    PORTC = 0x03;

    DDRD = 0xFF; // intialize to output
    PORTD = 0x00;

    // GCD_PERIOD = findGCD(sonarPeriod, displayPeriod);
    // GCD_PERIOD = findGCD(GCD_PERIOD, leftButtonPeriod);

    ADC_init();   // initializes ADC
    sonar_init(); // initializes sonar

    serial_init(9600);

    // TODO: Initialize tasks here
    unsigned char i = 0;
    tasks[i].state = SonarStart;
    tasks[i].period = sonarPeriod;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Sonar_Tick;
    ++i;
    tasks[i].state = DisplayStart;
    tasks[i].period = displayPeriod;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Display_Tick;
    ++i;
    tasks[i].state = buttonStart;
    tasks[i].period = leftButtonPeriod;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Button_L_Tick;
    ++i;
    tasks[i].state = redStart;
    tasks[i].period = RedPeriod;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Red_Tick;

    ++i;
    tasks[i].state = greenStart;
    tasks[i].period = RedPeriod;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Green_Tick;

    // tasks[0] = (task){
    //     SonarStart,      // Initial state of the task
    //     sonarPeriod,     // Task period
    //     tasks[0].period, // Initial elapsed time (can start at 0)
    //     &Sonar_Tick      // Pointer to the task's tick function
    // };
    // tasks[1] = (task){
    //     DisplayStart,    // Initial state of the task
    //     displayPeriod,   // Task period
    //     tasks[1].period, // Initial elapsed time (can start at 0)
    //     &Sonar_Tick      // Pointer to the task's tick function
    // };

    TimerSet(GCD_PERIOD);
    TimerOn();
    // int read = 0;
    while (1)
    {
        // sonarReading = sonar_read();

        // read = GetBit(PORTB, 0);
        // serial_println(sonarReading, 10);
    }

    return 0;
}
