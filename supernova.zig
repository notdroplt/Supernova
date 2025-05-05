const std = @import("std");
const testing = std.testing;
pub const Register = u4;

pub const Headers = struct {
    pub const Main = extern struct {
        /// Header magic
        magic: u64 = magicValue,

        /// Version necessary to run this code
        version: u64 = 0,

        /// CPU flags
        flags: u64 = 0,

        /// Code entry point
        entryPoint: u64 = 0,

        /// Count of memory regions
        memoryRegions: u64 = 0,

        /// Value to set magic to: "Zenithvm"
        pub const magicValue = 0x6D766874696E655A;
    };


    pub const MemoryMap  =  extern  struct  {
        /// Magic number
        magic: u64,

        /// Where does the mapping exist on the file, if MemoryFlags.Clear
        /// is set, only offset is read
        start: u64,
        
        /// Size of this memory map
        size: u64,

        /// Where should this map be placed
        offset: u64,

        flags: enum(u8) {
            /// Can read from this memory region
            Read = 0x01,

            /// Can write to this memory region
            Write = 0x02,

            /// Can execute code in this region
            Execute = 0x04,

            /// Allocate zero initialized memory
            Clear = 0x08
        },

        /// Value to set the magic field into
        pub const magicValue = 0x2170616D5f6D656D;
    };
};

// Allow better naming for registers before allocation
pub const InfiniteRegister = u16;



/// Supernova ISA opcodes
pub const Opcodes = enum(u8) {
    /// rd <- r1 & r2
    andr = 0x00, // R 

    /// add rd <- r1 & immediate
    andi = 0x01, // S 

    /// rd <- r1 ^ r2
    xorr = 0x02, // R 

    /// rd <- r1 ^ immediate
    xori = 0x03, // S 

    /// rd <- r1 | r2
    orr = 0x04, // R 

    /// rd <- r1 | immediate
    ori = 0x05, // S 

    /// rd <- ~r1
    not = 0x06, // R 

    /// rd <- popcount(r1)
    cnt = 0x07, // S 

    /// rd <- r1 << r2
    llsr = 0x08, // R 

    /// rd <- r1 << immediate
    llsi = 0x09, // S 

    /// rd <- r1 >> r2
    lrsr = 0x0A, // R 

    /// rd <- r1 >> immediate
    lrsi = 0x0B, // S 

    addr = 0x10, // R 
    addi = 0x11, // S 
    subr = 0x12, // R 
    subi = 0x13, // S 
    umulr = 0x14, // R 
    umuli = 0x15, // S 
    smulr = 0x16, // R 
    smuli = 0x17, // S 
    udivr = 0x18, // R 
    udivi = 0x19, // S 
    sdivr = 0x1A, // R 
    sdivi = 0x1B, // S 
    call = 0x1C, // R (deprecated)
    push = 0x1D, // S (deprecated)
    retn = 0x1E, // R (deprecated)
    pull = 0x1F, // S (deprecated)

    setgur = 0x20, // R
    setgui = 0x21, // S
    setgsr = 0x22, // R
    setgsi = 0x23, // S
    setleur = 0x24, // R
    setleui = 0x25, // S
    setlesr = 0x26, // R
    setlesi = 0x27, // S
    lui = 0x28, // L
    auipc = 0x29, // L
    pcall = 0x2A, // L
    pret = 0x2B, // L
    bout = 0x2C, // R
    out = 0x2D, // S
    bin = 0x2E, // R
    in = 0x2F, // S

    ld_byte = 0x30, // S
    ld_half = 0x31, // S
    ld_word = 0x32, // S
    ld_dwrd = 0x33, // S
    st_byte = 0x34, // S
    st_half = 0x35, // S
    st_word = 0x36, // S
    st_dwrd = 0x37, // S
    jal = 0x38, // L
    jalr = 0x39, // S
    je = 0x3A, // S
    jne = 0x3B, // S
    jgu = 0x3C, // S
    jgs = 0x3D, // S
    jleu = 0x3E, // S
    jles = 0x3F, // S

    // extension 1 - floating point
    flt_ldu = 0x40, // S
    flt_lds = 0x41, // S
    flt_stu = 0x42, // S
    flt_sts = 0x43, // S
    flt_add = 0x44, // R
    flt_sub = 0x45, // R
    flt_mul = 0x46, // R
    flt_div = 0x47, // R
    flt_ceq = 0x48, // R
    flt_cne = 0x49, // R
    flt_cgt = 0x4A, // R
    flt_cle = 0x4B, // R
    flt_rou = 0x4C, // R
    flt_flr = 0x4D, // R
    flt_cei = 0x4E, // R
    flt_trn = 0x4F, // R

    // the following instructions are only mnemonics for compiling
    // purposes, they are not real instructions
    
    /// reference a phi node 
    phi_node = 0xFF, // immediate: phi id

    /// jump to a different block
    blk_jmp = 0xFE, // r1: condition, r2: comparison register, rd + imm: offset (if any)

    /// move an intermediate to a register (differentiates from a normal ori)
    mov = 0xFD, // undefined
};

/// Register <- Register, Register instruction layout
pub const RInstruction align(64) = extern struct {
    /// Full instruction value
    instruction: u64,

    /// Create a new RInstruction
    pub fn init(opcode: Opcodes, reg1: Register, regd: Register, reg2: Register) RInstruction {
        const bit = @as(u64, @intFromEnum(opcode)) 
        | @as(u64, @intCast(reg1)) << 8
        | @as(u64, @intCast(regd)) << 12
        | @as(u64, @intCast(reg2)) << 16;

        return RInstruction{ 
            .instruction = bit
        };
    }

    /// Opcode getter
    pub fn op(self: RInstruction) Opcodes {
        return @enumFromInt(self.instruction & 0xFF);
    }

    /// Register 1 getter
    pub fn r1(self: RInstruction) Register {
        return @intCast((self.instruction >> 8) & 0xF);
    }

    /// Destination register getter
    pub fn rd(self: RInstruction) Register {
        return @intCast((self.instruction >> 12) & 0xF);
    }

    /// Register 2 getter
    pub fn r2(self: RInstruction) Register {
        return @intCast((self.instruction >> 16) & 0xF);
    }

    /// Get instruction as integer
    pub fn asInteger(self: RInstruction) u64 {
        return self.instruction;
    }

    /// Instruction integer constructor
    pub fn fromInteger(value: u64) RInstruction {
        return RInstruction{
            .instruction = value,
        };
    }
};


test RInstruction {
    const expectEqual = testing.expectEqual;

    const rInst = RInstruction.init(Opcodes.addr, 1, 2, 3);
    const value = rInst.asInteger();
    const rInst2 = RInstruction.fromInteger(value);

    try expectEqual(64, @bitSizeOf(RInstruction));

    try expectEqual(rInst2.op(), rInst.op());
    try expectEqual(Opcodes.addr, rInst.op());

    try expectEqual(rInst2.r1(), rInst.r1());
    try expectEqual(1, rInst.r1());

    try expectEqual(rInst2.rd(), rInst.rd());
    try expectEqual(2, rInst.rd());

    try expectEqual(rInst2.r2(), rInst.r2());
    try expectEqual(3, rInst.r2());
}


/// Register <- Register, immediate instruction layout
pub const SInstruction align(64) = extern struct {
    /// full instruction value
    instruction: u64,

    /// Create a new SInstruction
    pub fn init(opcode: Opcodes, reg1: Register, regd: Register, immed: u48) SInstruction {
        const bit = @as(u64, @intFromEnum(opcode)) 
        | @as(u64, @intCast(reg1)) << 8
        | @as(u64, @intCast(regd)) << 12
        | @as(u64, @intCast(immed)) << 16;

        return SInstruction{ 
            .instruction = bit
        };
    }

    /// Opcode getter
    pub fn op(self: SInstruction) Opcodes {
        return @enumFromInt(self.instruction & 0xFF);
    }

    /// Register 1 getter
    pub fn r1(self: SInstruction) Register {
        return @intCast((self.instruction >> 8) & 0xF);
    }

    /// Destination register getter
    pub fn rd(self: SInstruction) Register {
        return @intCast((self.instruction >> 12) & 0xF);
    }

    /// Register 2 signed getter
    pub fn imm(self: SInstruction) u64 {
        return @bitCast((self.instruction >> 16) & 0xFFFFFFFFFFFF);
    }

    /// Immediate unsigned getter
    pub fn uimm(self: SInstruction) u48 {
        return @intCast((self.instruction >> 16) & 0xFFFFFFFFFFFF);
    }

    /// transform an instruction into an integer
    pub fn asInteger(self: SInstruction) u64 {
        return self.instruction;
    }

    /// transform an integer into an instruction
    pub fn fromInteger(value: u64) SInstruction {
        return SInstruction{
            .instruction = value,
        };
    }
};

test SInstruction {
    const expectEqual = testing.expectEqual;

    const sInst = SInstruction.init(Opcodes.addi, 1, 2, 3);
    const value = sInst.asInteger();
    const sInst2 = SInstruction.fromInteger(value);

    try expectEqual(64, @bitSizeOf(SInstruction));

    try expectEqual(sInst2.op(), sInst.op());
    try expectEqual(Opcodes.addi, sInst.op());

    try expectEqual(sInst2.r1(), sInst.r1());
    try expectEqual(1, sInst.r1());

    try expectEqual(sInst2.rd(), sInst.rd());
    try expectEqual(2, sInst.rd());

    try expectEqual(sInst2.imm(), sInst.imm());
    try expectEqual(3, sInst.imm());
}

/// Register-immediate instruction layout
pub const LInstruction align(64) = extern struct {
    /// Full instruction value
    instruction: u64,

    /// Create a new LInstruction
    pub fn init(opcode: Opcodes, reg1: Register, immed: u52) LInstruction {
        const bit = @as(u64, @intFromEnum(opcode)) 
        | @as(u64, @intCast(reg1)) << 8
        | @as(u64, @intCast(immed)) << 12;

        return LInstruction{ 
            .instruction = bit
        };
    }

    /// Opcode getter
    pub fn op(self: LInstruction) Opcodes {
        return @enumFromInt(self.instruction & 0xFF);
    }

    /// Register 1 getter
    pub fn r1(self: LInstruction) Register {
        return @intCast((self.instruction >> 8) & 0xF);
    }

    /// Immediate signed getter
    pub fn imm(self: LInstruction) i64 {
        return @bitCast((self.instruction >> 12) & 0xFFFFFFFFFFFFF);
    }

    /// Immediate unsigned getter
    pub fn uimm(self: LInstruction) u64 {
        return @intCast((self.instruction >> 12) & 0xFFFFFFFFFFFFF);
    }

    pub fn asInteger(self: LInstruction) u64 {
        return self.instruction;
    }

    pub fn fromInteger(value: u64) LInstruction {
        return LInstruction{
            .instruction = value
        };
    }
};

test LInstruction {
    const expectEqual = testing.expectEqual;

    const lInst = LInstruction.init(Opcodes.jal, 1, 2);
    const value = lInst.asInteger();
    const lInst2 = LInstruction.fromInteger(value);

    try expectEqual(64, @bitSizeOf(LInstruction));

    try expectEqual(lInst2.op(), lInst.op());
    try expectEqual(Opcodes.jal, lInst.op());

    try expectEqual(lInst2.r1(), lInst.r1());
    try expectEqual(1, lInst.r1());

    try expectEqual(lInst2.imm(), lInst.imm());
    try expectEqual(2, lInst.imm());
}

pub const Instruction = union(enum) {
    RInst: RInstruction,
    SInst: SInstruction,
    LInst: LInstruction,
};

pub const BlockJumpCondition = enum(u8) {
    Unconditional,
    Equal,
    NotEqual,
    Greater,
    LessEqual,
};

pub const ConfigFlags = enum(u16) {
    page  = 0x0001,
    stack = 0x0002,
    idiv  = 0x0004,
    int   = 0x0008,
    flt   = 0x0010,
    fence = 0x0020,
    cset  = 0x0040,
    cmove = 0x0080,
    m64   = 0x0100,
    m128  = 0x0200,
    m256  = 0x0400,
    m512  = 0x0800,
    ioint = 0x1000,
    host  = 0x2000,
};

pub const ProcessorCall = enum(u52) {
    /// Processor defined functions
    Functions = std.math.maxInt(u52),

    /// Division by zero
    DivisionByZero = 0,

    /// Undiagnosed Fault
    GeneralFault = 1,

    /// Processor failed to recover from a fault
    DoubleFault = 2,

    /// Processor failed to recover from a double fault (irrecoverable)
    TripleFault = 3,

    /// Invalid instruction
    InvalidInstruction = 4,

    /// Invalid page access
    PageFault = 5,

    /// Halt request
    Halt = 6,

    /// Nothing happened, default state
    NormalExecution = 7
};

/// Reason why thread was destroyed
pub const ThreadDestruction = error {
    /// Thread was destroyed normally
    ProgramEnd,

    /// Thread was destroyed by a fault
    CorruptedMemory,

    /// Thread was destroyed by a fault
    InterruptCrashLoop
};


/// Thread model
pub const ThreadModel = extern struct {
    /// Config flags
    flags: u16,

    /// Interrupt count,
    interrupts: u64,

    /// Page level
    pageLevel: u8,

    /// Page size
    pageSize: u64,

    /// Model identifier string
    modelName: [4]u64,

    /// IO address space
    ioAddressSpace: u64,

    /// Last instruction index
    lastInstruction: u64,

    /// Current implementation flags
    pub const default = @intFromEnum(ConfigFlags.cset) 
                      | @intFromEnum(ConfigFlags.idiv) 
                      | @intFromEnum(ConfigFlags.int) 
                      | @intFromEnum(ConfigFlags.host)
                      | @intFromEnum(ConfigFlags.stack);
};

pub const defaultModel = ThreadModel{
    .flags = ThreadModel.default,
    .interrupts = 1 << 52 - 1,
    .pageLevel = 0,
    .pageSize = 0,

    // SupernovaCommunityVirtualMachine
    .modelName = [_]u64{ 
        0x766f6e7265707553,
        0x696e756d6d6f4361,
        0x6175747269567974,
        0x656e696863614d6c
    },
    .ioAddressSpace = 0,
    .lastInstruction = @intFromEnum(Opcodes.jles)
};

/// Runable thread
pub const Thread = extern struct {
    /// program call arguments register
    pub const pcallRegister = 15;

    /// program call first return register
    pub const pcallReturn1st = 14;

    /// program call second return register
    pub const pcallReturn2nd = 13;

    /// program call invalid opcode register
    pub const pcallInvalidOpcode = 14;

    /// Register count
    pub const registerCount = 16;

    /// Inaccessible registers
    pub const specialRegisterCount = 6;

    /// Index of special registers
    specials : extern struct {
        /// Current instruction pointer
        instructionPointer: u64 = 0,

        /// Start of interrupt vector
        interruptVector: u64 = 0,

        /// Main page pointer
        pagePointer : u64 = 0,

        /// Current handled pcall
        currentPcall : u64 = 0,

        /// Address to return after pcall
        pcallReturn: u64 = 0,

        /// Thread flags
        flags: u64 = 0,
    },

    registers: [Thread.registerCount]u64,
    model: ThreadModel,
    memory: [*]u8,
    memory_size: usize,

    pub fn init(mem: []u8, tModel: ThreadModel, entry: u64) Thread {
        return Thread{
            .registers = undefined,
            .specials = .{
                .instructionPointer = entry,
            },
            .memory = mem.ptr,
            .memory_size = mem.len,
            .model = tModel,
        };
    }

    pub inline fn fetch(self: *Thread, comptime Size: type, address: u64) Size {
        @setCold(false);
        return @as([*]const Size, @alignCast(@ptrCast(self.memory)))[address];
    }

    pub inline fn place(self: *Thread, comptime Size: type, address: u64, value: Size) void {
        @setCold(false);
        @as([*]Size, @alignCast(@ptrCast(self.memory)))[address] = value;
    }

    fn pcallMinusOne(self: *Thread) !void {
        switch (self.registers[pcallRegister]) {
            0x0000000000000000 => {
                self.registers[pcallReturn1st] = 2;
                self.registers[pcallReturn2nd] = self.model.interrupts;
            },
            0x0000000000000001 => {
                self.specials.interruptVector = self.registers[pcallReturn1st];
            },
            0x0000000100000000 => {
                self.registers[pcallReturn1st] = self.model.pageLevel;
                self.registers[pcallReturn2nd] = self.model.pageSize;
            },
            0x0000000300000000 => {
                self.registers[pcallReturn1st] = 1;
            },
            0x0000000300000001 => {
                return ThreadDestruction.ProgramEnd;
            },
            else => {
                self.registers[pcallReturn1st] = 0;
            }
        }
    }

    fn dispatchPcall(self: *Thread, pcall: u52) ThreadDestruction!void {
        if (pcall == @intFromEnum(ProcessorCall.Functions)) {
            return self.pcallMinusOne();
        }

        if (self.specials.currentPcall == @intFromEnum(ProcessorCall.DoubleFault)) {
            return ThreadDestruction.InterruptCrashLoop;
        }

        if (self.specials.currentPcall != @intFromEnum(ProcessorCall.NormalExecution)) {
            self.specials.currentPcall = @intFromEnum(ProcessorCall.DoubleFault);
            return;
        }
        
        self.specials.currentPcall = pcall;
        self.specials.pcallReturn = self.specials.instructionPointer + @sizeOf(Instruction);
        self.specials.instructionPointer = 
            self.fetch(u64, self.specials.interruptVector + pcall * @sizeOf(Instruction));
    }

    fn invalidInstruction(self: *Thread, instruction: u64) !void {
        @setCold(false);
        self.registers[pcallInvalidOpcode] = instruction;
        try self.dispatchPcall(@intFromEnum(ProcessorCall.InvalidInstruction));
    }

    fn do_andr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] & self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_andi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] & inst.uimm();
        self.registers[inst.rd()] = res;
    }

    fn do_xorr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] ^ self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_xori(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] ^ inst.uimm();
        self.registers[inst.rd()] = res;
    }

    fn do_orr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] | self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_ori(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] | inst.uimm();
        self.registers[inst.rd()] = res;
    }

    fn do_not(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        self.registers[inst.rd()] = ~self.registers[inst.r1()];
    }

    fn do_cnt(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        self.registers[inst.rd()] = @popCount(self.registers[inst.r1()]);
    }

    fn do_llsr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] << @truncate(self.registers[inst.r2()]);
        self.registers[inst.rd()] = res;
    }

    fn do_llsi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] << @truncate(inst.uimm());
        self.registers[inst.rd()] = res;
    }

    fn do_lrsr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] >> @truncate(self.registers[inst.r2()]);
        self.registers[inst.rd()] = res;
    }

    fn do_lrsi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        self.registers[inst.rd()] = self.registers[inst.r1()] >> @truncate(inst.uimm());
    }

    fn do_addr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] + self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_addi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] + inst.uimm();
        self.registers[inst.rd()] = res;
    }

    fn do_subr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] - self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_subi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        self.registers[inst.rd()] = self.registers[inst.r1()] - inst.uimm();
    }
    
    fn do_umulr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] * self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_umuli(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] * inst.uimm();
        self.registers[inst.rd()] = res;
    }

    fn do_smulr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] * self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_smuli(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] * inst.imm();
        self.registers[inst.rd()] = res;
    }

    fn do_udivr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] / self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_udivi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] / inst.uimm();
        self.registers[inst.rd()] = res;
    }

    fn do_sdivr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] / self.registers[inst.r2()];
        self.registers[inst.rd()] = res;
    }

    fn do_sdivi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] / inst.imm();
        self.registers[inst.rd()] = res;
    }

    fn do_call(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = RInstruction.fromInteger(instr);
        const stack = self.registers[inst.r1()];
        const base = self.registers[inst.r2()];
        const addr = self.registers[inst.rd()];

        self.place(u64, stack, base);
        self.place(u64, stack + @sizeOf(Instruction), 
            self.specials.instructionPointer + @sizeOf(Instruction));

        self.registers[inst.r1()] += 2 * @sizeOf(Instruction);

        self.specials.instructionPointer = addr;
    }

    fn do_push(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = SInstruction.fromInteger(instr);
        const stack = self.registers[inst.r1()];
        const addr = self.registers[inst.rd()];

        self.place(u64, stack, addr);
        self.registers[inst.r1()] += @sizeOf(Instruction);
    }

    fn do_retn(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = RInstruction.fromInteger(instr);
        const stack = self.registers[inst.r1()] - 2 * @sizeOf(Instruction);

        self.registers[inst.r1()] -= 2 * @sizeOf(Instruction);
        self.registers[inst.r2()] = self.fetch(u64, stack);
        self.specials.instructionPointer = self.fetch(u64, stack + @sizeOf(Instruction));
    }

    fn do_pull(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(true);
        const inst = SInstruction.fromInteger(instr);
        const stack = self.registers[inst.r1()] - 1 * @sizeOf(Instruction);

        self.registers[inst.r1()] -= @sizeOf(Instruction);
        self.registers[inst.rd()] = self.fetch(u64, stack);
    }

    fn do_ldbyte(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.registers[inst.rd()] = self.fetch(u8, addr);
    }

    fn do_ldhalf(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.registers[inst.rd()] = self.fetch(u16, addr);
    }

    fn do_ldword(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.registers[inst.rd()] = self.fetch(u32, addr);
    }

    fn do_lddwrd(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.registers[inst.rd()] = self.fetch(u64, addr);
    }

    fn do_stbyte(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.place(u8, addr, @intCast(self.registers[inst.rd()] & 0xFF));
    }

    fn do_sthalf(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.place(u16, addr, @intCast(self.registers[inst.rd()] & 0xFFFF));
    }

    fn do_stword(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.place(u32, addr, @intCast(self.registers[inst.rd()] & 0xFFFFFFFF));
    }

    fn do_stdwrd(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.registers[inst.r1()] + inst.imm();
        self.place(u64, addr, self.registers[inst.rd()]);
    }

    fn do_jal(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = LInstruction.fromInteger(instr);
        const addr = self.specials.instructionPointer + @as(u64, @intCast(inst.imm() * 3));
        self.registers[inst.r1()] = self.specials.instructionPointer + @sizeOf(Instruction);
        self.specials.instructionPointer = addr;
    }

    fn do_jalr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const addr = self.specials.instructionPointer + self.registers[inst.rd()] + inst.imm() * 3;
        self.registers[inst.r1()] = self.specials.instructionPointer + @sizeOf(Instruction);
        self.specials.instructionPointer = addr;
    }

    fn do_je(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        if (self.registers[inst.r1()] == self.registers[inst.rd()])
            self.specials.instructionPointer += inst.imm() * 3;
    }
    
    fn do_jne(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        if (self.registers[inst.r1()] != self.registers[inst.rd()])
            self.specials.instructionPointer += inst.imm() * 3;
    }
    
    fn do_jgu(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        if (self.registers[inst.r1()] > self.registers[inst.rd()])
            self.specials.instructionPointer += inst.imm() * 3;
    }
    
    fn do_jgs(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        if (@as(i64, @bitCast(self.registers[inst.r1()])) > 
            @as(i64, @bitCast(self.registers[inst.rd()])))
            self.specials.instructionPointer += inst.imm() * 3;
    }

    fn do_jleu(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        if (self.registers[inst.r1()] <= self.registers[inst.rd()])
            self.specials.instructionPointer += inst.imm() * 3;
    }
    fn do_jles(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        if (@as(i64, @bitCast(self.registers[inst.r1()])) <= 
            @as(i64, @bitCast(self.registers[inst.rd()])))
            self.specials.instructionPointer += inst.imm() * 3;
    }

    fn do_setgur(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] > self.registers[inst.r2()];
        self.registers[inst.rd()] = @intFromBool(res);
    }
    
    fn do_setgui(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] > inst.uimm();
        self.registers[inst.rd()] = @intFromBool(res);
    }

    fn do_setgsr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = @as(i64, @bitCast(self.registers[inst.r1()])) 
                  > @as(i64, @bitCast(self.registers[inst.r2()]));
        self.registers[inst.rd()] = @intFromBool(res);
    }

    fn do_setgsi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = @as(i64, @bitCast(self.registers[inst.r1()])) 
                  > inst.imm();
        self.registers[inst.rd()] = @intFromBool(res);
    }

    fn do_setleur(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] <= self.registers[inst.r2()];
        self.registers[inst.rd()] = @intFromBool(res);
    }
    
    fn do_setleui(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = self.registers[inst.r1()] <= inst.uimm();
        self.registers[inst.rd()] = @intFromBool(res);
    }

    fn do_setlesr(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        const res = @as(i64, @bitCast(self.registers[inst.r1()])) 
                  <= @as(i64, @bitCast(self.registers[inst.r2()]));
        self.registers[inst.rd()] = @intFromBool(res);
    }

    fn do_setlesi(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = SInstruction.fromInteger(instr);
        const res = @as(i64, @bitCast(self.registers[inst.r1()])) 
                  <= inst.imm();
        self.registers[inst.rd()] = @intFromBool(res);
    }

    fn do_lui(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = LInstruction.fromInteger(instr);
        self.registers[inst.r1()] = inst.uimm() << 12;
    }

    fn do_auipc(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = LInstruction.fromInteger(instr);
        self.registers[inst.r1()] = self.specials.instructionPointer + inst.uimm() << 12;
    }

    fn do_pcall(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = LInstruction.fromInteger(instr);
        try self.dispatchPcall(@truncate(inst.uimm()));
    }

    fn do_pret(self: *Thread, instr: u64) ThreadDestruction!void {
        @setCold(false);
        const inst = RInstruction.fromInteger(instr);
        self.specials.instructionPointer = self.registers[inst.r1()];
        self.registers[inst.r1()] = self.registers[inst.r2()];
    }

    const opcodeFunction = *const fn (*Thread, u64) ThreadDestruction!void;

    inline fn generateTable(self: *Thread) []const opcodeFunction {
        const flags = self.model.flags;
        return &[_]opcodeFunction{
            do_andr,
            do_andi,
            do_xorr,
            do_xori,
            do_orr,
            do_ori,
            do_not,
            do_cnt,
            do_llsr,
            do_llsi,
            do_lrsr,
            do_lrsi,
            invalidInstruction,
            invalidInstruction,
            invalidInstruction,
            invalidInstruction,

            do_addr,
            do_addi,
            do_subr,
            do_subi,
            do_umulr,
            do_umuli,
            do_smulr,
            do_smuli,
            if (flags & @intFromEnum(ConfigFlags.idiv) != 0) do_udivr else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.idiv) != 0) do_udivi else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.idiv) != 0) do_sdivr else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.idiv) != 0) do_sdivi else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.stack) != 0) do_call else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.stack) != 0) do_push else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.stack) != 0) do_retn else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.stack) != 0) do_pull else invalidInstruction,

            do_ldbyte,
            do_ldhalf,
            do_ldword,
            do_lddwrd,
            do_stbyte,
            do_sthalf,
            do_stword,
            do_stdwrd,
            do_jal,
            do_jalr,
            do_je,
            do_jne,
            do_jgu,
            do_jgs,
            do_jleu,
            do_jles,

            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setgur else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setgui else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setgsr else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setgsi else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setleur else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setleui else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setlesr else invalidInstruction,
            if (flags & @intFromEnum(ConfigFlags.cset) != 0) do_setlesi else invalidInstruction,
            do_lui,
            do_auipc,
            do_pcall,
            do_pret,
            invalidInstruction, // todo: implement I/O efficiently
            invalidInstruction, // todo: implement I/O efficiently
            invalidInstruction, // todo: implement I/O efficiently
            invalidInstruction, // todo: implement I/O efficiently
        };
    }

    /// Execute an instruction
    fn execute(self: *Thread, array: []const opcodeFunction) ThreadDestruction!void {
        self.registers[0] = 0; // zero register
        const inst = self.fetch(u64, self.specials.instructionPointer);
        const opcode = inst & 0xFF;

        if (opcode > self.model.lastInstruction) {
            return dispatchPcall(self, @intFromEnum(ProcessorCall.InvalidInstruction));
        }

        try array[opcode](self, inst);
        self.specials.instructionPointer += @sizeOf(Instruction);
    }

    pub fn loadArgs(self: *Thread, argv: [][]u8) void {
        const argvstart = self.memory.len - argv.len * @sizeOf(u64);
        var curroff = argvstart;
        for (0..argv.len) |i| {
            const arg = argv[argv.len - i - 1];
            const argvOffset = (curroff - arg.len) & ~4; // 4 byte alignment
            const argvLenOffset = argvOffset - 4;

            curroff = argvLenOffset;
            self.place(u32, argvLenOffset, arg.len);

            for (0..arg.len) |j| {
                self.memory[argvOffset + j + 4] = arg.ptr[j];
            }

            const argvIdx = argvstart + (argv.len - i - 1) * @sizeOf(u64);
            self.place(u32, argvIdx, argvLenOffset);
        }
    }

    pub fn step(self: *Thread, argv: [][]u8) !u64 {
        const array = self.generateTable();
        self.loadArgs(argv);
        
        self.registers[0] = 0; // zero register
        self.registers[pcallReturn1st] = argv.len;
        self.registers[pcallReturn2nd] = argv.ptr;

        try self.execute(array);

        return self.registers[1];
    }

    pub fn run(self: *Thread) ThreadDestruction!u64 {
        const array = self.generateTable();

        while (true) {
            self.execute(array) catch |v| {
                if (v == ThreadDestruction.ProgramEnd) break;
                return v;
            };
        }

        return self.registers[1];
    }

};


test "Thread.fetch and Thread.place" {
    const expectEqual = testing.expectEqual;

    const memory = try testing.allocator.alloc(u8, 8);
    const model = ThreadModel{
        .flags 
          = @intFromEnum(ConfigFlags.cset) 
          | @intFromEnum(ConfigFlags.idiv) 
          | @intFromEnum(ConfigFlags.int) 
          | @intFromEnum(ConfigFlags.host)
          | @intFromEnum(ConfigFlags.stack),
        .interrupts = 1 << 52 - 1,
        .ioAddressSpace = 0,
        .modelName = .{0x53757065724e6f76, 0x6154657374696e67, 0, 0},
        .pageSize = 0,
        .pageLevel = 1,
        .lastInstruction = @intFromEnum(Opcodes.jles),
    };

    defer testing.allocator.free(memory);

    var thread = Thread.init(memory, model, 0);

    thread.place(u64, 0, 0x1122334455667788);

    try expectEqual(0x11, thread.fetch(u8, 0x7));
    try expectEqual(0x88, thread.fetch(u8, 0x0));
    try expectEqual(0x1122334455667788, thread.fetch(u64, 0x0));    
}