#!/usr/bin/python3

from fusesoc.capi2.generator import Generator
from litex.build.tools import write_to_file
from litex.build.tools import replace_in_file
from litex.build.generic_platform import *
from litex.build.xilinx import XilinxPlatform
from litex.build.lattice import LatticePlatform
from litex.soc.integration.builder import *
from litedram.gen import *
import subprocess
import os
import sys
import yaml
import shutil

def make_new_dir(base, added):
    r = os.path.join(base, added)
    if os.path.exists(r):
        shutil.rmtree(r)
    os.mkdir(r)
    return r
    
gen_src_dir = os.path.dirname(os.path.realpath(__file__))
base_dir = os.path.join(gen_src_dir, os.pardir)
build_top_dir = make_new_dir(base_dir, "build")
gen_src_dir = os.path.join(base_dir, "gen-src")
gen_dir = make_new_dir(base_dir, "generated")

# Build the init code for microwatt-initialized DRAM
#
# XXX Not working yet
#
def build_init_code(build_dir):

    # More path fudging
    sw_dir = os.path.join(build_dir, "software");
    sw_inc_dir = os.path.join(sw_dir, "include")
    gen_inc_dir = os.path.join(sw_inc_dir, "generated")
    src_dir = os.path.join(gen_src_dir, "sdram_init")
    lxbios_src_dir = os.path.join(soc_directory, "software", "bios")
    lxbios_inc_dir = os.path.join(soc_directory, "software", "include")
    print("     sw dir:", sw_dir)
    print("gen_inc_dir:", gen_inc_dir)
    print("    src dir:", src_dir)
    print(" lx src dir:", lxbios_src_dir)
    print(" lx inc dir:", lxbios_inc_dir)

    # Generate mem.h
    mem_h = "#define MAIN_RAM_BASE 0x40000000"
    write_to_file(os.path.join(gen_inc_dir, "mem.h"), mem_h)

    # Environment
    env_vars = []
    def _makefile_escape(s):  # From LiteX
        return s.replace("\\", "\\\\")
    def add_var(k, v):
        env_vars.append("{}={}\n".format(k, _makefile_escape(v)))

    add_var("BUILD_DIR", sw_dir)
    add_var("SRC_DIR", src_dir)
    add_var("GENINC_DIR", sw_inc_dir)
    add_var("LXSRC_DIR", lxbios_src_dir)
    add_var("LXINC_DIR", lxbios_inc_dir)
    write_to_file(os.path.join(gen_inc_dir, "variables.mak"), "".join(env_vars))

    # Build init code
    print(" Generating init software...")
    makefile = os.path.join(src_dir, "Makefile")
    r = subprocess.check_call(["make", "-C", build_dir, "-I", gen_inc_dir, "-f", makefile])
    print("Make result:", r)

    return os.path.join(sw_dir, "obj", "sdram_init.hex")

def generate_one(t, mw_init):

    print("Generating target:", t)

    # Muck with directory path
    build_dir = make_new_dir(build_top_dir, t)
    t_dir = make_new_dir(gen_dir, t)

    # Grab config file
    cfile = os.path.join(gen_src_dir, t  + ".yml")
    core_config = yaml.load(open(cfile).read(), Loader=yaml.Loader)

    ### TODO: Make most stuff below a function in litedram gen.py and
    ###       call it rather than duplicate it
    ###

    # Convert YAML elements to Python/LiteX
    for k, v in core_config.items():
        replaces = {"False": False, "True": True, "None": None}
        for r in replaces.keys():
            if v == r:
                core_config[k] = replaces[r]
        if "clk_freq" in k:
            core_config[k] = float(core_config[k])
        if k == "sdram_module":
            core_config[k] = getattr(litedram_modules, core_config[k])
        if k == "sdram_phy":
            core_config[k] = getattr(litedram_phys, core_config[k])

    # Override values for mw_init
    if mw_init:
        core_config["cpu"] = None
        core_config["csr_align"] = 64

    # Generate core
    if core_config["sdram_phy"] in [litedram_phys.ECP5DDRPHY]:
        platform = LatticePlatform("LFE5UM5G-45F-8BG381C", io=[], toolchain="trellis")
    elif core_config["sdram_phy"] in [litedram_phys.A7DDRPHY, litedram_phys.K7DDRPHY, litedram_phys.V7DDRPHY]:
        platform = XilinxPlatform("", io=[], toolchain="vivado")
    else:
        raise ValueError("Unsupported SDRAM PHY: {}".format(core_config["sdram_phy"]))

    soc      = LiteDRAMCore(platform, core_config, integrated_rom_size=0x6000, csr_data_width=32)

    # Build into build_dir
    builder  = Builder(soc, output_dir=build_dir, compile_gateware=False)
    vns      = builder.build(build_name="litedram_core", regular_comb=False)

    # Grab generated gatewar dir
    gw_dir = os.path.join(build_dir, "gateware")

    # Generate init-cpu.txt if any and generate init code if none
    cpu = core_config["cpu"]
    if mw_init:
        src_wrap_file = os.path.join(gen_src_dir, "wrapper-mw-init.vhdl")
        src_init_file = build_init_code(build_dir)
    else:
        write_to_file(os.path.join(t_dir, "init-cpu.txt"), cpu)
        src_wrap_file = os.path.join(gen_src_dir, "wrapper-self-init.vhdl")
        src_init_file = os.path.join(gw_dir, "mem.init")

    # Copy generated files to target dir, amend them if necessary
    core_file = os.path.join(gw_dir, "litedram_core.v")
    dst_init_file = os.path.join(t_dir, "litedram_core.init")
    dst_wrap_file = os.path.join(t_dir, "litedram-wrapper.vhdl")
    replace_in_file(core_file, "mem.init", "litedram_core.init")
    shutil.copy(core_file, t_dir)
    shutil.copyfile(src_init_file, dst_init_file)    
    shutil.copyfile(src_wrap_file, dst_wrap_file)

def main():

    targets = ['arty','nexys-video']

    # XXX Set mw_init to False to use a local VexRiscV for memory inits
    for t in targets:
        generate_one(t, mw_init = True)

    # XXX TODO: Remove build dir unless told not to via cmdline option
    
if __name__ == "__main__":
    main()
