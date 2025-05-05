const std = @import("std");
const sn = @import("supernova.zig");

fn printHelp(output: anytype) !void {
    const string =
    \\ Supernova: Zenith virtual machine runtime
    \\ usage: snvm [options] [flags] -- [executable args]
    \\
    \\ options:
    \\  -h --help           | display this help
    \\  -v --version        | print current version
    \\  -p --properties     | get current virtual machine properties
    \\  --run [path]        | run file specified by path
    \\
    \\ ---- config flags ----
    \\
    \\ --thread-count=[count]   | amount of threads to have, defaults to 1, max 32
    \\ --start-thread=[id]      | which thread to start, no real efect changing but defaults to 1
    \\ --add-search-path [path] | path to find new modules, defaults to `.`, `./snmod` and [INSTALLDIR]/snmod
    \\ --add-module [name]      | add a module by name
    \\
    \\ ---- sandbox flags ----
    \\
    \\ --memory-limit=[size][prefix]    | allocate memory to at most `size` bytes, prefixes need to be one of: b, k[b], m[b], g[b]
    \\ --load-modules=(true|false)      | all hypervisor requests to load a library in the host will forcefully fail
    \\ --enable-[instr]=(true|false)    | tune certain instructions, instr can be any of: div, int, float, ioint, stack
    \\
    \\ note: --enable-stack is the only option which already defaults to false
    \\
    ;

    try output.print(string, .{}); 
}

fn printProperties(output: anytype) !void {
    const string =
    \\Properties:"
    \\==================="
    \\thread model: Community
    \\ 
    \\====================================="
    \\instruction group implementations:"
    \\    group 0: fully implemented
    \\    group 1: fully implemented
    \\    group 2: fully implemented
    \\    group 3: no i/o
    \\    group 4: not implemented
    \\    group 5: not implemented
    \\    group 6: not implemented
    \\==============================
    \\ pcall -1:\n"
    \\    t0:0 -> r31 = 2, r30 = 2^51 - 1
    \\    t0:1 implemented
    \\    t1:0 -> r31 = 0 paging not yet implemented
    \\    t2:0 -> r31 = 0 
    \\
    ;
    try output.print(string, .{}); 
}

fn strStartsWith(big: []const u8, small: []const u8) bool {
    if (small.len != big.len) return false;
    for (small, big) |c1, c2 | 
        if (c1 != c2) return false;
        
    return true;
}

fn getSize(str: []const u8) u64 {
    var result: u64 = 0;
    for (str) |c|
        switch (c) {
            'b', 'B' => return result,
            'k', 'K' => result *= 1000,
            'm', 'M' => result *= 1000000,
            'g', 'G' => result *= 1000000000,
            't', 'T' => result *= 1000000000000,
            '0'...'9' => result = result * 10 + c - '0',
            else => return 0,
        };
    return result; 
}

const Arguments = struct {
    memoryLimit: u64,
    shouldEnable: u16,
    filename: [:0]const u8,
    loadLibs: bool,
    shouldContinue: bool
};

fn loadArgv(writer: anytype, alloc: std.mem.Allocator) !Arguments {
    var args: Arguments = undefined;
        
    const argv = try std.process.argsAlloc(alloc);
    defer std.process.argsFree(alloc, argv);

    if (argv.len == 1) {
        args.shouldContinue = false;
        try printHelp(writer);
        return args;
    }

    for (argv, 0..) |cur_arg, i| {
        
        if (strStartsWith(cur_arg, "-h") or strStartsWith(cur_arg, "--help")) {
            args.shouldContinue = false;
            try printHelp(writer);
            break;
        }
        
        if (strStartsWith(cur_arg, "-v") or strStartsWith(cur_arg, "--version")) {
            std.debug.print("v0.0.0.0\n",.{});
            args.shouldContinue = false;
            break;
        }
        
        if (strStartsWith(cur_arg, "-p") or strStartsWith(cur_arg, "--properties")) {
            args.shouldContinue = false;
            try printProperties(writer);
            break;
        }
        
        if (strStartsWith(cur_arg, "--run")) {
            if (i + 1 < argv.len) {
                args.filename = argv[i + 1];
                continue;
            }
            std.debug.print("run file was not specified\n", .{});
        }
        
        if (strStartsWith(cur_arg, "--memory-limit=")) {
            const new_str = cur_arg[14..];
            const value: u64 = getSize(new_str);
            args.memoryLimit = value;
            continue;
        }
        
        if (strStartsWith(cur_arg, "--load-modules=")) {
            args.loadLibs =  std.mem.eql(u8, cur_arg[14..], "true");
            continue;
        }
        
        if (strStartsWith(cur_arg, "--enable-")) {
            const equals = std.mem.indexOfScalar(u8, cur_arg, '=').?;
            const element = cur_arg[9..equals];
            
            const names = [_][]const u8{ "div", "int", "float", "ioint", "stack" };
            const flags = [_]u16{
                @intFromEnum(sn.ConfigFlags.cset),
                @intFromEnum(sn.ConfigFlags.idiv),
                @intFromEnum(sn.ConfigFlags.int),
                @intFromEnum(sn.ConfigFlags.host),
                @intFromEnum(sn.ConfigFlags.stack)
            };
            
            for (names, 0..) |name, j| {
                if (std.mem.eql(u8, element, name)) {
                    const value = cur_arg[equals + 1 ..];
                    const flag_val = flags[j];
                    
                    if (std.mem.eql(u8, value, "true")) {
                        args.shouldEnable |= flag_val;
                    } else {
                        args.shouldEnable &= ~flag_val;
                    }
                    break;
                }
            }
            
            continue;
        }
    }
    
    return args;
}

const read_status = error{
    FileNotFound,
    InvalidHeader,
    InvalidEntryPoint,
    VersionMismatch,
    MagicMismatch,
    InvalidMemoryRegion,
    FileError
};

const readReturn = struct {
    memory: []u8,
    entry: u64,
};

fn loadFile(args: Arguments, alloc: std.mem.Allocator) !readReturn {
    var file = try std.fs.cwd().openFile(args.filename, .{});
    defer file.close();

    var reader = file.reader();

    const size = (try file.stat()).size;
    if (size < @sizeOf(sn.Headers.Main))
        return read_status.InvalidHeader;

    const header = try reader.readStruct(sn.Headers.Main);

    if (header.magic != sn.Headers.Main.magicValue)
        return read_status.MagicMismatch;

    const memoryMaps = try alloc.alloc(sn.Headers.MemoryMap, header.memoryRegions);
    defer alloc.free(memoryMaps);

    var sum : usize = 0;
    for (memoryMaps) |mmap|{
        if (mmap.magic != sn.Headers.MemoryMap.magicValue)
            continue;
        sum += mmap.size;
    }

    const memory = try alloc.alloc(u8, sum);

    return readReturn{
        .memory = memory,
        .entry = header.entryPoint
    };
}

pub fn main() !u8 {
    const stdout_file = std.io.getStdOut().writer();
    var bw = std.io.bufferedWriter(stdout_file);


    const stdout = bw.writer();

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};

    const alloc = gpa.allocator();

    defer _ = gpa.deinit();
    
    const args = try loadArgv(stdout, alloc);
    try bw.flush();

    if (!args.shouldContinue) {
        return 0;
    }

    const ent= try loadFile(args, alloc);
    defer alloc.free(ent.memory);

    var thread = sn.Thread.init(ent.memory, sn.defaultModel, ent.entry);
    const res = thread.run();

    if (res) |v| {
        return @truncate(v);
    
    } else |err| {
        try stdout.print( "=== exception {s} caused the virtual machine to halt unexpectedly\n", .{@errorName(err)});
        return 1;
    }

    try bw.flush();
}