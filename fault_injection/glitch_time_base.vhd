----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    20:41:54 05/14/2022 
-- Design Name: 
-- Module Name:    glitch - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_arith.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity glitch is
    Port ( a : in  STD_LOGIC;
           clk : in  STD_LOGIC;
           glitch_out : out  STD_LOGIC);
end glitch;

architecture Behavioral of glitch is
signal const:integer:=320;
signal TempENC:integer:=10;
signal s:STD_LOGIC:='0';
signal cnt:integer:=0;
signal cnt_round:integer:=0;
signal div:integer:=0;
signal cnt_ENC:integer:=0;
signal cnt2:integer:=0;
signal glitch_T:integer:=0;
signal ENC_T:integer:=0;
--signal q:STD_LOGIC_vector(31 downto 0);
begin

process(clk)
	begin
		if (rising_edge(clk)) then
			if (ENC_T<TempENC  and a='1') then
				cnt<=cnt+1;
				s<='1';
			elsif (ENC_T<TempENC  and a='0' and s='1') then
				ENC_T<=ENC_T+1;
				div<=cnt;
				s<='0';
			elsif (ENC_T=TempENC  and a='0') then
				if div>const then
					cnt_ENC<=cnt_ENC+1;
					div<=div-(const);
				elsif div<const+1  then
					ENC_T<=ENC_T+1;
				end if;
			elsif(ENC_T=TempENC+1 and a='1') then
				if cnt2<cnt_ENC then
					cnt2<=cnt2+1;
				elsif cnt2=cnt_ENC then
					cnt_round<=cnt_round+1;
					if glitch_T<4 and cnt_round=30 then
						glitch_T<=glitch_T+1;
						glitch_out<='1';
						cnt_round<=0;
					else
						cnt2<=0;
					end if;
				end if;				
			elsif(ENC_T=TempENC+1 and a='0') then
				glitch_out<='0';
				cnt2<=0;
				glitch_T<=0;
				cnt_round<=0;
			end if;
		end if;
	end process;
end Behavioral;
