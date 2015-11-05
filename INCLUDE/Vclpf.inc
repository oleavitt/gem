// Voltage Controlled Low Pass Filter.

include "knobs.inc"
include "jacks.inc"
include "panels.inc"

define VCLPF object
{
	Panel_2_Installed;
	Knob_1_00
	{
		rotate <0, 0, 180>;
		translate <1, 7.5, Panel_Thickness + 0.1>;
	}
	Knob_0_75
	{
		rotate <0, 0, 105>;
		translate <1, 6, Panel_Thickness + 0.1>;
	}
	Knob_0_75
	{
		rotate <0, 0, 83>;
		translate <1, 4.5, Panel_Thickness + 0.1>;
	}
	JackBlack { translate <0.5, 2.25, Panel_Thickness>; }
	JackRed { translate <1.5, 2.25, Panel_Thickness>; }
	JackBlack { translate <0.5, 1.25, Panel_Thickness>; }
	JackBlack { translate <1.5, 1.25, Panel_Thickness>; }
	bound box <-0.001, -0.001, -0.001>, <2, 9, 0.75>;
}