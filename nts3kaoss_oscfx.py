from hvccnts3kaoss import LogueSDKV2Generator

class Nts3_oscfx(LogueSDKV2Generator):
    FIXED_PARAMS = ("pitch",)
    BUILTIN_PARAMS = ("pitch_note", "touch_began", "touch_moved", "touch_ended", "touch_stationary", "touch_cancelled", "noteon_trig", "noteoff_trig")
    UNIT_NUM_OUTPUT = 2
    MAX_SDRAM_SIZE = 3145728
    MAX_UNIT_SIZE = 32768
    MSG_POOL_ON_SRAM = True

    def unit_type():
        return "genfx"
