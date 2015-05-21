library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity main is
    Port ( 
        -- signals connected to MCU
        XPIO        : in  std_logic;        -- on rising edge will put INT to L
        XDMA        : in  std_logic;        -- on rising edge will put DRQ to L
        XRnW        : in  std_logic;        -- defines data direction (1: DATA1 <- DATA2,  0: DATA1 -> DATA2)
        XCMD        : out std_logic;        -- this is combination of CS and A1, will go low on 1st cmd byte from ACSI port
        reset_hans  : in  std_logic;        -- this is the signal which resets Hans, and it will reset this CPLD too to init it
        XATN        : out std_logic;        -- just a copy of ATN signal

        -- signals connected to MCU, which should be just copies of SCSI port states
        XRESET  : out std_logic;
        XCS     : out std_logic;
        XACK    : out std_logic;

        -- signals connected to SCSI port
        SRST : in    std_logic;            -- RESET device
        ATN  : in    std_logic;            -- ATN - when MSG OUT phase should start
        SEL  : in    std_logic;            -- target selection is marked with this signal
        BSYa : inout std_logic;            -- when device is listening
        BSYb : out   std_logic;            -- when device is listening
        DBPa : out   std_logic;            -- odd parity bit for data
        DBPb : out   std_logic;            -- odd parity bit for data

        IOa  : out   std_logic;            -- I/O - input / output
        IOb  : out   std_logic;            -- I/O - input / output
        CDa  : out   std_logic;            -- C/D - command / data
        CDb  : out   std_logic;            -- C/D - command / data
        SMSGa: out   std_logic;            -- MSG - message phase
        SMSGb: out   std_logic;            -- MSG - message phase

        SREQa: out   std_logic;            -- REQ goes from device...
        SREQb: out   std_logic;            -- REQ goes from device...
        SACK : in    std_logic;            -- ...and initiator answers with ACK

        -- DATA1 is connected to SCSI port, DATA2 is data latched on CS and ACK and connected to MCU
        DATA1a    : inout std_logic_vector(7 downto 0);
        DATA1b    : out   std_logic_vector(7 downto 0);

        DATA2     : inout std_logic_vector(7 downto 0);

        -- the following is 2-to-1 Multiplexer for connecting both MCUs to single RX pin (used for FW update)
        TXSEL1n2: in std_logic;         -- TX select -    1: TX_out <- TX_Franz,    0: TX_out <- TX_Hans
        TX_Franz: in  std_logic;        -- TX from Franz
        TX_Hans : in  std_logic;        -- TX from Hans
        TX_out  : out std_logic;        -- muxed TX

        clk     : in std_logic;

        -- used for real HW type identification
        HDD_IF  : in std_logic          -- 0 when ACSI, 1 when SCSI 
        ) ;
end main;

architecture Behavioral of main is
    type SCSI_states   is (FREE, DATA_OUT, DATA_IN, COMMAND, STATUS, MESSAGE_OUT, MESSAGE_IN);

    signal phaseReset    : std_logic;
    signal phaseClock    : std_logic;

    signal REQtrig       : std_logic;
    signal REQtrigPrev   : std_logic;
    signal REQstate      : std_logic;

    signal DATA1latch    : std_logic_vector(7 downto 0);
    signal resetCombo    : std_logic;
    signal identify      : std_logic;

    signal busState      : SCSI_states;

    signal CDsignal      : std_logic;
    signal MSGsignal     : std_logic;
    signal IOsignal      : std_logic;

    signal oddParity     : std_logic;
    signal BSYsignal     : std_logic;

    signal d1out         : std_logic;

    signal identifyA     : std_logic;
    signal identifyS     : std_logic;

    signal nSelection    : std_logic;
    signal lathClock     : std_logic;

    signal doPhaseStatus : std_logic;
    signal doPhaseCommand: std_logic;
    signal doPhaseDataOut: std_logic;
    signal doPhaseDataIn : std_logic;

begin

    identify   <= XPIO and XDMA and TXSEL1n2;   -- when TXSEL1n2 selects Franz (='1') and you have PIO and DMA pins high, then you can read the identification byte from DATA2
    identifyA  <= identify and (not HDD_IF);    -- active when IDENTIFY and it's ACSI hardware
    identifyS  <= identify and HDD_IF;          -- active when IDENTIFY and it's SCSI hardware

    phaseReset <= SRST and reset_hans and (not identify);  -- when one of these goes low, reset phase to FREE
    phaseClock <= XPIO xor XDMA;                -- if one of these (but not both) go high, it's time to change the phase

    nSelection <= not ((not SEL) and BSYa);     -- selection is when SEL is 0 and BSY is 1. nSelection is inverted selection, because DATA1latch is captured on falling edge
    XCMD       <= nSelection or (not SRST);     -- falling edge means target selection (SRST must be 1, otherwise ignored)

    resetCombo <= SRST and reset_hans and (not identify);   -- when one of these reset signals is low, the result is low

    doPhaseCommand  <= (phaseClock and XPIO and (not XRnW));
    doPhaseStatus   <= (phaseClock and XPIO and (    XRnW));
    doPhaseDataOut  <= (phaseClock and XDMA and (not XRnW));
    doPhaseDataIn   <= (phaseClock and XDMA and (    XRnW));

-- TODO: message phase is totally skipped - is it needed, will it work?
   
    fsm: process(clk, phaseReset) is
        variable doReq        : boolean := false;
        variable statusWasRead: boolean := false;
    begin
        if(rising_edge(clk)) then
            REQtrigPrev <= REQtrig;             -- store previous state
            REQtrig     <= XPIO xor XDMA;       -- current trigger state - trigger REQ if one of these goes high, but not both! (that would be identify cmd)
        end if;

        if (phaseReset = '0') then
            busState      <= FREE;
            REQstate      <= '1';
            doReq         := false;
            statusWasRead := false;
        elsif(rising_edge(clk)) then
            if    (doPhaseStatus = '1'  and (busState /= STATUS))     then      -- if need to change phase, don't do anything else in this cycle
                busState <= STATUS;
                doReq    := true;
                REQstate <= '1';
            elsif (doPhaseCommand = '1' and (busState /= COMMAND))    then      -- if need to change phase, don't do anything else in this cycle
                busState <= COMMAND;
                doReq    := true;
                REQstate <= '1';
            elsif (doPhaseDataIn = '1'  and (busState /= DATA_IN))    then      -- if need to change phase, don't do anything else in this cycle
                busState <= DATA_IN;
                doReq    := true;
                REQstate <= '1';
            elsif (doPhaseDataOut = '1' and (busState /= DATA_OUT))   then      -- if need to change phase, don't do anything else in this cycle
                busState <= DATA_OUT;
                doReq    := true;
                REQstate <= '1';
            else                                                                -- no need to change phase? 
                if(SACK = '0' or resetCombo = '0') then                         -- if we're reseting REQ, do it
                    REQstate <= '1';
                elsif ((REQtrigPrev = '0' and REQtrig = '1') or doReq = true) then  -- on rising edge or REQtrig (on XPIO / XDMA, or from previous phase change), or when should do REQtrig because it was triggered by phase change
                    doReq    := false;
                    REQstate <= '0';
                end if;

                if (busState = STATUS) then                     -- in STATUS state
                    if (SACK = '0') then                        -- when currently reading status (when ACK is 0), mark that status was read
                        statusWasRead := true;
                    elsif (SACK = '1' and statusWasRead) then   -- if ACK is back to 1, and status was read (it was 0 for a while), go to FREE state
                        busState      <= FREE;
                        REQstate      <= '1';
                        doReq         := false;
                        statusWasRead := false;
                    end if;
                end if;    
            end if;
            
        end if;
    end process;

    SREQa <= '0' when REQstate='0' else 'Z';             -- REQ - pull to L, otherwise hi-Z
    SREQb <= '0' when REQstate='0' else 'Z';             -- REQ - pull to L, otherwise hi-Z

    -- 8-bit latch register
    -- latch data from ST on falling edge of lathClock (that means either on falling nSelection, or falling SACK)
    lathClock <= SACK and nSelection;

    dataLatch: process(lathClock) is
    begin 
        if (falling_edge(lathClock)) then
            DATA1latch <= not DATA1a;                    -- store inverted value to latch - SCSI SE has inverted data
        end if;
    end process;

    -- d1out says whether should output DATA1 -- only when XRnW is 1 (read) and resetCombo is 1 (not in reset state), otherwise don't drive DATA1 pins
    d1out <= '1' when ((XRnW='1' ) and (resetCombo='1')) else '0';  

    -- DATA1a and DATA1b are connected to SCSI bus, data goes out when going from MCU to ST (READ operation) and it's not reset state (this means d1out is 1)
    -- Also output inverted version of DATA2, because SCSI SE has inverted data
    DATA1a(7) <= '0' when ((d1out='1') and (DATA2(7)='1')) else 'Z';
    DATA1b(7) <= '0' when ((d1out='1') and (DATA2(7)='1')) else 'Z';
    DATA1a(6) <= '0' when ((d1out='1') and (DATA2(6)='1')) else 'Z';
    DATA1b(6) <= '0' when ((d1out='1') and (DATA2(6)='1')) else 'Z';
    DATA1a(5) <= '0' when ((d1out='1') and (DATA2(5)='1')) else 'Z';
    DATA1b(5) <= '0' when ((d1out='1') and (DATA2(5)='1')) else 'Z';
    DATA1a(4) <= '0' when ((d1out='1') and (DATA2(4)='1')) else 'Z';
    DATA1b(4) <= '0' when ((d1out='1') and (DATA2(4)='1')) else 'Z';
    DATA1a(3) <= '0' when ((d1out='1') and (DATA2(3)='1')) else 'Z';
    DATA1b(3) <= '0' when ((d1out='1') and (DATA2(3)='1')) else 'Z';
    DATA1a(2) <= '0' when ((d1out='1') and (DATA2(2)='1')) else 'Z';
    DATA1b(2) <= '0' when ((d1out='1') and (DATA2(2)='1')) else 'Z';
    DATA1a(1) <= '0' when ((d1out='1') and (DATA2(1)='1')) else 'Z';
    DATA1b(1) <= '0' when ((d1out='1') and (DATA2(1)='1')) else 'Z';
    DATA1a(0) <= '0' when ((d1out='1') and (DATA2(0)='1')) else 'Z';
    DATA1b(0) <= '0' when ((d1out='1') and (DATA2(0)='1')) else 'Z';

    -- DATA2 is connected to Hans (STM32 mcu), data goes out when going from ST to MCU (WRITE operation)
    DATA2 <=    "ZZZZZ0ZZ"  when TXSEL1n2='0'    else   -- when TXSEL1n2 selects Hans, we're writing to Hans's flash, we need bit DATA2.2 (bit #2) to be 0 (it's BOOT1 bit on STM32 MCU)
                "00100010"  when identifyS='1'   else   -- GOOD: when identify condition met, this identifies the XILINX and HW revision (0010 - HW rev 2, 0 - it's SCSI HW, 010 - SCSI Xilinx FW)
                "00101010"  when identifyA='1'   else   -- BAD : when identify condition met, this identifies the XILINX and HW revision (0010 - HW rev 2, 1 - it's ACSI HW, 010 - SCSI Xilinx FW)
                DATA1latch  when XRnW='0'        else   -- when set in WRITE direction, output latched DATA1 to DATA2 
                "ZZZZZZZZ";                             -- otherwise don't drive this

    -- TX_out is connected to RPi, and this is multiplexed TX pin from two MCUs
    TX_out <=   TX_Franz when TXSEL1n2='1' else TX_Hans;   -- depending on TXSEL1n2 switch TX_out to TX_Franz or TX_Hans

    -- just copy state from one signal to another
    XCS    <= SACK;
    XACK   <= SACK;
    XRESET <= SRST;
    XATN   <= ATN;

    -- setting of C/D, MSG, I/O states depending on BUS STATE
    phaseSignals: process(busState) is
    begin
        if    (busState = FREE) then
            CDsignal  <= '1';
            MSGsignal <= '1';
            IOsignal  <= '1';
        elsif (busState = STATUS) then
            CDsignal  <= '0';
            MSGsignal <= '1';
            IOsignal  <= '0';
        elsif (busState = COMMAND) then
            CDsignal  <= '0';
            MSGsignal <= '1';
            IOsignal  <= '1';
        elsif (busState = DATA_IN) then
            CDsignal  <= '1';
            MSGsignal <= '1';
            IOsignal  <= '0';
        elsif (busState = DATA_OUT) then
            CDsignal  <= '1';
            MSGsignal <= '1';
            IOsignal  <= '1';
        else
            -- This last 'else' is here to avoid using latches for C/D, MSG, I/O signals 
            -- They shouldn't be latched, just display the BUS STATE to these signals. 
            CDsignal  <= '1';
            MSGsignal <= '1';
            IOsignal  <= '1';
        end if;    
    end process;

    -- these should be set according to the current SCSI phase
    CDa   <= '0' when CDsignal ='0' else 'Z';
    CDb   <= '0' when CDsignal ='0' else 'Z';

    SMSGa <= '0' when MSGsignal='0' else 'Z';
    SMSGb <= '0' when MSGsignal='0' else 'Z';

    IOa   <= '0' when IOsignal ='0' else 'Z';
    IOb   <= '0' when IOsignal ='0' else 'Z';

    -- odd parity output for IN direction
    oddParity <= ( (DATA2(0) xor DATA2(1)) xor (DATA2(2) xor DATA2(3)) ) xor ( (DATA2(4) xor DATA2(5)) xor (DATA2(6) xor DATA2(7)) );
    DBPa      <= '0' when ((IOsignal='0') and (oddParity='0')) else 'Z';        -- when in *_IN phase and parity is 0, then set it to 0; otherwise let it in 'Z'
    DBPb      <= '0' when ((IOsignal='0') and (oddParity='0')) else 'Z';        -- when in *_IN phase and parity is 0, then set it to 0; otherwise let it in 'Z'

    -- pull BSY low when needed, otherwise just let it in Hi-Z
    -- setting BSY signal depending on BUS STATE
    BSYsignal <= '1' when busState=FREE else '0';       -- when in FREE phase, don't pull BSY low, in other phases - pull BSY low
    BSYa      <= '0' when BSYsignal='0' else 'Z';
    BSYb      <= '0' when BSYsignal='0' else 'Z';

end Behavioral;
