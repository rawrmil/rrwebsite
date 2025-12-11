// --- ByteReader/ByteWriter (stream I/O) ---
class ByteReader {
	constructor(buffer) {
		this.dv = new DataView(buffer);
		this.i = 0;
	}
	u8()   { const r = this.dv.getUint8(this.i);        this.i += 1; return r; }
	u16()  { const r = this.dv.getUint16(this.i, true); this.i += 2; return r; }
	u32()  { const r = this.dv.getUint32(this.i, true); this.i += 4; return r; }
	u64()  { const r = this.dv.getUint64(this.i, true); this.i += 8; return r; }
	str(n) {
		const str = new TextDecoder().decode(new Uint8Array(this.dv.buffer, this.i, n));
		this.i += n;
		return str;
	}
	strn() {
		var n = this.u32();
		return this.str(n);
	}
}
class ByteWriter {
	constructor() {
		this.arr = [];
		this.dv = new DataView(new ArrayBuffer(8))
	}
	bytes() { return new Uint8Array(this.arr); }
	u8(v)  { this.arr.push(v); }
	u16(v) { this.dv.setUint16(0, v, true); this.arr.push(...new Uint8Array(this.dv.buffer, 0, 2)); }
	u32(v) { this.dv.setUint16(0, v, true); this.arr.push(...new Uint8Array(this.dv.buffer, 0, 4)); }
	u64(v) { this.dv.setUint16(0, v, true); this.arr.push(...new Uint8Array(this.dv.buffer, 0, 8)); }
	str(s) {
		const te = new TextEncoder().encode(s);
		this.arr.push(...te);
	}
	strn(s) {
		const te = new TextEncoder().encode(s);
		this.u32(te.length);
		this.arr.push(...te);
	}
}
