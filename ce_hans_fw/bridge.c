#include "bridge.h"
#include "timers.h"

extern BYTE brStat;                                                         // status from bridge
extern BYTE isAcsiNotScsi;
extern BYTE lastScsiStatusByte;

BYTE pioReadFailed;

void resetXilinx(void);

void timeoutStart(void)
{
    // init the timer 4, which will serve for timeout measuring 
    TIM_Cmd(TIM3, DISABLE);     // disable timer
    TIM3->CNT   = 0;            // set timer value to 0
    TIM3->SR    = 0xfffe;       // clear UIF flag
    TIM_Cmd(TIM3, ENABLE);      // enable timer
}   

BYTE timeout(void)
{
    if((TIM3->SR & 0x0001) != 0) {  // overflow of TIM4 occured?
        TIM3->SR = 0xfffe;          // clear UIF flag
        return TRUE;
    }
    
    return FALSE;
}

BYTE PIO_gotFirstCmdByte(void)
{
    WORD exti = EXTI->PR;                                                   // Pending register (EXTI_PR)
        
    if(exti & aCMD ) {
        return TRUE;
    }
    
    return FALSE;
}

// get 1st CMD byte from ST  -- without setting INT
BYTE PIO_writeFirst(void)
{
    BYTE val;
    
    timeoutStart();                                                         // start the timeout timer
    ACSI_DATADIR_WRITE();                                                   // data as inputs (write)
    
    // init vars, 
    brStat = E_OK;                                                          // init bridge status to E_OK
    EXTI->PR = aCMD | aCS;                                                  // clear int for 1st CMD byte (aCMD) and also int for CS, because also that one will be set

    val = GPIOB->IDR;                                                       // read the data
    return val;
}

// get next CMD byte from ST -- with setting INT to LOW and waiting for CS 
BYTE PIO_write(void)
{
    BYTE val = 0;

    // create rising edge on aPIO
    GPIOA->BSRR = aPIO;                                                     // aPIO to HIGH
    __asm  { nop }
    GPIOA->BRR  = aPIO;                                                     // aPIO to LOW

    while(1) {                                                              // wait for CS or timeout
        WORD exti = EXTI->PR;
        
        if(exti & aCS) {                                                    // if CS arrived
            val = GPIOB->IDR;                                               // read the data
            break;
        }
        
        if(timeout()) {                                                     // if timeout happened
            LOG_ERROR(40);
            brStat = E_TimeOut;                                             // set the bridge status
            break;
        }
    }
    
    EXTI->PR = aCS;                                                         // clear int for CS
    return val;
}

// send status byte to host, and on SCSI also to MSG IN byte
void PIO_read(BYTE scsiStatusByte)
{
    pioReadFailed = FALSE;                      // didn't fail (yet)

    if(brStat != E_TimeOut) {                   // if we didn't have bridge timeout, we can try to send STATUS byte
        lastScsiStatusByte = scsiStatusByte;    // store last SCSI status byte - for debugging purpose
        PIO_read_solely(scsiStatusByte);        // this sends only STATUS byte to host - both in ACSI and SCSI
    }
    
    if(!isAcsiNotScsi) {                        // if it's SCSI, send also MSG IN to host
        if(brStat != E_TimeOut) {               // if we didn't have bridge timeout, we can try to send MSG IN
            MSG_read(0);
        }
    }
    
    if(brStat != E_OK) {                        // if some timeout occured, then failed
        pioReadFailed = TRUE;
    }

    resetXilinx();              // reset XILINX - put BSY, C/D, I/O in released states - needed for SCSI, doesn't harm anything in ACSI
    ACSI_DATADIR_WRITE();       // data as inputs (write)
}

void PIO_read_solely(BYTE val)
{
    ACSI_DATADIR_READ();                                                    // data as outputs (read)
    GPIOB->ODR = val;                                                       // write the data to output data register
    
    // create rising edge on aPIO
    GPIOA->BSRR = aPIO;                                                     // aPIO to HIGH
    __asm  { nop }
    GPIOA->BRR  = aPIO;                                                     // aPIO to LOW

    while(1) {                                                              // wait for CS or timeout
        WORD exti = EXTI->PR;
        
        if(exti & aCS) {                                                    // if CS arrived
            break;
        }
        
        if(timeout()) {                                                     // if timeout happened
            LOG_ERROR(41);
            brStat = E_TimeOut;                                             // set the bridge status
            break;
        }
    }
    
    EXTI->PR = aCS;                                                         // clear int for CS
}

// send MESSAGE IN byte to ST 
void MSG_read(BYTE val)
{
    ACSI_DATADIR_READ();                                                    // data as outputs (read)
    GPIOB->ODR = val;                                                       // write the data to output data register
    
    // create rising edge on XMSG
    GPIOC->BSRR = XMSG;                                                     // XMSG to HIGH
    __asm  { nop }
    GPIOC->BRR  = XMSG;                                                     // XMSG to LOW

    while(1) {                                                              // wait for CS or timeout
        WORD exti = EXTI->PR;
        
        if(exti & aCS) {                                                    // if CS arrived
            break;
        }
        
        if(timeout()) {                                                     // if timeout happened
            LOG_ERROR(43);
            brStat = E_TimeOut;                                             // set the bridge status
            break;
        }
    }
    
    EXTI->PR = aCS;                                                         // clear int for CS
}

void DMA_read(BYTE val)
{
    GPIOB->ODR = val;                                                       // write the data to output data register
    
    // create rising edge on aDMA
    GPIOA->BSRR = aDMA;                                                     // aDMA to HIGH
    __asm  { nop }
    GPIOA->BRR  = aDMA;                                                     // aDMA to LOW

    while(1) {                                                              // wait for ACK or timeout
        WORD exti = EXTI->PR;
        
        if(exti & aACK) {                                                   // if ACK arrived
            break;
        }
        
        if(timeout()) {                                                     // if timeout happened
            LOG_ERROR(44);
            brStat = E_TimeOut;                                             // set the bridge status
            break;
        }
    }
    
    EXTI->PR = aACK;                                                        // clear int for ACK
}

BYTE DMA_write(void)
{
    WORD exti;
    BYTE val = 0;
    WORD cnt=0;
    
    // create rising edge on aDMA
    GPIOA->BSRR = aDMA;                                                     // aDMA to HIGH
    __asm  { nop }
    GPIOA->BRR  = aDMA;                                                     // aDMA to LOW

    while(1) {                                                              // wait for ACK or timeout
        cnt++;
        
        if(cnt > 3) {
            GPIOB->BSRR = (1 << 12); // hi
        }
        
        exti = EXTI->PR;
        
        if(exti & aACK) {                                                   // if ACK arrived
            val = GPIOB->IDR;                                               // read the data
            break;
        }
        
        if(timeout()) {                                                     // if timeout happened
            LOG_ERROR(45);
            brStat = E_TimeOut;                                             // set the bridge status
            break;
        }
    }
    
    EXTI->PR = aACK;                                                        // clear int for ACK
    return val;
}
