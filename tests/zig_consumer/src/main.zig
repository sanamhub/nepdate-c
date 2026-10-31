const std = @import("std");
const c = @import("c");

pub fn main() !void {
    var date: c.nepdate_date = undefined;
    var bs: c.nepdate_ymd = undefined;
    if (c.nepdate_from_ad(2024, 7, 30, &date) != c.NEPDATE_OK) {
        return error.NepdateConversionFailed;
    }
    if (c.nepdate_to_bs(date, &bs) != c.NEPDATE_OK) {
        return error.NepdateConversionFailed;
    }
    if (c.nepdate_patro_public_holidays(null, 0) == 0) {
        return error.NepdatePatroMissing;
    }
    const year: u32 = @intCast(bs.year);
    const month: u32 = @intCast(bs.month);
    const day: u32 = @intCast(bs.day);
    std.debug.print("BS {d:0>4}-{d:0>2}-{d:0>2}\n", .{ year, month, day });
}
