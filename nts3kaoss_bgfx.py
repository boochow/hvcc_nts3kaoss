from hvccloguesdk import LogueSDKV2Generator

class Nts3_bgfx(LogueSDKV2Generator):
    FIXED_PARAMS = ()
    BUILTIN_PARAMS = ("touch_began", "touch_moved", "touch_ended", "touch_stationary", "touch_cancelled")
    UNIT_NUM_OUTPUT = 2
    MAX_SDRAM_SIZE = 3145728
    MAX_UNIT_SIZE = 32768
    MSG_POOL_ON_SRAM = True

    def unit_type():
        return "genfx"
