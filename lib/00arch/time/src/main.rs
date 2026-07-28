#![no_std]

#[unsafe(no_mangle)]                                                                                                                                                            
static mut TIME_BASE: u64 = 0;

#[cfg(SHYOS_BACKEND_QEMU_VIRT)]
#[unsafe(no_mangle)]
//输入时间后触发时钟中断
unsafe extern "C" fn set_timer(ms: u64){
sbi_rt::set_timer(riscv::register::time::read() as u64 + ms*10000);
}

#[cfg(SHYOS_BACKEND_QEMU_VIRT)]
#[unsafe(no_mangle)]
///返回开机后的毫秒
unsafe extern "C" fn get_time()->u64{
    unsafe{
        (riscv::register::cycle::read64()-TIME_BASE)/10_000
    }
}


#[cfg(SHYOS_BACKEND_QEMU_VIRT)]
#[unsafe(no_mangle)]
unsafe extern "C" fn init_time(){
    unsafe {TIME_BASE = riscv::register::cycle::read64();}
}