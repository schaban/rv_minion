export fn fib(_x: u32) u32 {
	var x = _x;
	if (x >= 2) {
		x = fib(x - 1) + fib(x - 2);
	}
	return x;
}

export fn _start() void {
	asm volatile("ebreak");
}
