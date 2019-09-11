library ieee;
use ieee.std_logic_1164.all;

entity toplevel is
    generic (
	MEMORY_SIZE   : positive := 524288;
	RAM_INIT_FILE : string   := "firmware.hex";
	RESET_LOW     : boolean := true
	);
    port(
	ext_clk   : in  std_ulogic;
	ext_rst   : in  std_ulogic;

	-- UART0 signals:
	uart0_txd : out std_ulogic;
	uart0_rxd : in  std_ulogic;

	-- NIA out on GPIOs
	ja        : out std_ulogic_vector(7 downto 0);
	jb        : out std_ulogic_vector(7 downto 0)
	);
end entity toplevel;

architecture behaviour of toplevel is

    -- Reset signals:
    signal soc_rst : std_ulogic;
    signal pll_rst_n : std_ulogic;

    -- Internal clock signals:
    signal system_clk : std_ulogic;
    signal system_clk_locked : std_ulogic;

    signal nia : std_ulogic_vector(61 downto 0);
begin
    ja <= nia(7 downto 0);
    jb <= nia(15 downto 8);

    reset_controller: entity work.soc_reset
	generic map(
	    RESET_LOW => RESET_LOW
	    )
	port map(
	    ext_clk => ext_clk,
	    pll_clk => system_clk,
	    pll_locked_in => system_clk_locked,
	    ext_rst_in => ext_rst,
	    pll_rst_out => pll_rst_n,
	    rst_out => soc_rst
	    );

    clkgen: entity work.clock_generator
	port map(
	    ext_clk => ext_clk,
	    pll_rst_in => pll_rst_n,
	    pll_clk_out => system_clk,
	    pll_locked_out => system_clk_locked
	    );

    -- Main SoC
    soc0: entity work.soc
	generic map(
	    MEMORY_SIZE   => MEMORY_SIZE,
	    RAM_INIT_FILE => RAM_INIT_FILE,
	    RESET_LOW     => RESET_LOW,
	    SIM           => false
	    )
	port map (
	    system_clk        => system_clk,
	    rst               => soc_rst,
	    uart0_txd         => uart0_txd,
	    uart0_rxd         => uart0_rxd,
	    nia_out           => nia
	    );

end architecture behaviour;
