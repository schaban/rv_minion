const std = @import("std");

export fn fib(_x: u32) u32 {
	var x = _x;
	if (x >= 2) {
		x = fib(x - 1) + fib(x - 2);
	}
	return x;
}

const PI_F: f32 = std.math.pi;

export fn sin_s(x: f32) f32 {
	var sgn: f32 = 1.0;
	var val = x;
	if (x < 0.0) {
		sgn = -1.0;
		val = -x;
	}
	if (val > @as(f32, (1<<24)-1)) {
		return 0.0;
	}
	var ir: u32 = @intFromFloat(val * (4.0 / PI_F));
	ir += ir & 1;
	const fr: f32 = @floatFromInt(ir);
	ir &= 7;
	if (ir > 3) {
		ir -= 4;
		sgn = -sgn;
	}
	if (val > @as(f32, 1 << 13)) {
		val = val - (fr * (PI_F / 4.0));
	} else {
		val -= fr * 0.78515625;
		val -= fr * 0.000241875648;
		val -= fr * 3.77489506e-8;
	}
	const s = val * val;
	var y: f32 = switch (ir) {
		1, 2 => ((s*2.44331568e-5 - 0.00138873165)*s + 0.0416666456) * s*s - s*0.5 + 1.0,
		else => ((0.00833216123 - s*0.000195152956)*s - 0.166666552)*s*val + val,
	};
	y *= sgn;
	return y;
}

export fn _start() void {
	asm volatile("ebreak");
}
