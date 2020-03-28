ZMQ    MACRO
       LDQ     =0 
ZMQ    END  
* A...B.  WORD IS THE ARGUMENT(TXH A,,B),  AND THE REST IS AS FOLLOWS-- 
*     
*      GLIMS   WORD,HOLD,BUMP,CHECK 
*      ...  
* HOLD AXC     --,1     
* X    BSS  
*      ...  
* CHECK        ---           *+2,1,--     
* BUMP TXI     X,1,--   
*     
* THE IR SPECIFIED IS ALWAYS -THIS WORD.  
*     
GLIMS  MACRO   WORD,HOLD,BUMP,CHECK,D1,D2,D3    
       CAL     WORD     
       STD     D1 
       AXT     1,1
       PAX     ,2 
D1     TXL     D2,2,1   
       AXT     -1,1     
D2     BSS  
       IRP     BUMP     
       SCD     BUMP,1   
       IRP  
       PAX     ,2 
       IRP     HOLD     
       SXA     HOLD,2   
       IRP  
       PDX     ,2 
       LDQ     =O700000000000 
       TXL     D3,1,1   
       TXI     *+1,2,1  
       LDQ     =O300000000000 
D3     BSS  
       XCL  
       IRP     CHECK    
       SCD     CHECK,2  
       STP     CHECK    
       IRP  
GLIMS  END  
REPORT MACRO   Z,TEXT,CR01,CR02,CR03
       TRA     CR01     
       BCI     Z,TEXT   
CR01   BSS  
       TSX     $WRFLX,4 
               CR01-Z,,Z
REPORT END  
