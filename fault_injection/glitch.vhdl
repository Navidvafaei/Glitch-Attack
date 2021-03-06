entity glitching is
    Port ( start_sig : in  STD_LOGIC;
           clk : in  STD_LOGIC;
			  test: out  STD_LOGIC;
           Glitch_out : out  STD_LOGIC);
end glitching;

architecture Behavioral of glitching is

signal cnt:integer:=0;
signal cnt1:integer:=0;
signal cnt2:integer:=0;

begin

process(clk)
	begin
		if (rising_edge(clk)) then
			if (start_sig='1') then
				cnt<=cnt+1;
				cnt2<=0;
				if (cnt>2500 and cnt1<100) then
					Glitch_out<='1';
					cnt1<=cnt1+1; 
				elsif (cnt>2500  and cnt1>=100) then
					Glitch_out<='0';
					test<='0';
				end if;
			elsif (start_sig='0') then 
				cnt2<=cnt2+1;
				if (cnt2>=5000) then 
					cnt<=0;
					cnt1<=0;
				end if;
			end if;
		end if;
	end process;
end Behavioral;
