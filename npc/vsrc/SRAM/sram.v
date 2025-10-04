`timescale 1ns / 1ps
//****************************************VSCODE PLUG-IN**********************************//
//----------------------------------------------------------------------------------------
// IDE :                   VSCODE     
// VSCODE plug-in version: Verilog-Hdl-Format-3.5.20250220
// VSCODE plug-in author : Jiang Percy
//----------------------------------------------------------------------------------------
//****************************************Copyright (c)***********************************//
// Copyright(C)            IMECAS
// All rights reserved     
// File name:              
// Last modified Date:     2025/03/01 14:47:29
// Last Version:           V1.0
// Descriptions:           
//----------------------------------------------------------------------------------------
// Created by:             Cui Junyan
// Created date:           2025/03/01 14:47:29
// mail      :             1354315077@qq.com 
// Version:                V1.0
// TEXT NAME:              sram.v
// PATH:                   ~/ysyx-workbench/npc/vsrc/SRAM/sram.v
// Descriptions:           
//                         
//----------------------------------------------------------------------------------------
//****************************************************************************************//
/* function test */
import "DPI-C" function int npc_pmem_read(int addr);
import "DPI-C" function void npc_pmem_write(int addr, int wdata,byte wmask);

module sram(
    input                                  clock                      ,
    input                                  reset                      ,

    output                                 logic                             awready,
    input                                  logic                             awvalid,
    input                                  logic         [  31: 0]           awaddr,
    input                                  logic         [   3: 0]           awid,
    input                                  logic         [   7: 0]           awlen,
    input                                  logic         [   2: 0]           awsize,
    input                                  logic         [   1: 0]           awburst,
    //写地址通道
    output                                 logic                             wready,
    input                                  logic                             wvalid,
    input                                  logic         [  31: 0]           wdata,
    input                                  logic         [   3: 0]           wstrb,
    input                                  logic                             wlast,
    //写数据通道
    input                                  logic                            bready,
    output                                 logic                            bvalid,
    output                                 logic        [   1: 0]           bresp,
    output                                 logic        [   3: 0]           bid,
    //写响应通道
    output                                 logic                            arready,
    input                                  logic                            arvalid,
    input                                  logic        [  31: 0]           araddr,
    input                                  logic        [   3: 0]           arid,
    input                                  logic        [   7: 0]           arlen,
    input                                  logic        [   2: 0]           arsize,
    input                                  logic        [   1: 0]           arburst,
    //读地址通道
    input                                  logic                            rready,
    output                                 logic                            rvalid,
    output                                 logic        [   1: 0]           rresp,
    output                                 logic        [  31: 0]           rdata,
    output                                 logic                            rlast,
    output                                 logic        [   3: 0]           rid 
    //读数据通道
);
    localparam                          idle                       = 2'b00 ; //空闲
    localparam                          write                      = 2'b01 ; //写状态
    localparam                          addr                       = 2'b10 ; //地址状态

    reg                [   1: 0]        state                       ; //状态
    reg               [   31: 0]        mem_wdata                   ; //写数据
    wire               [   7: 0]        wmask                       ; //写掩码

    assign                              arready                     = (state == idle);
    assign                              awready                     = (state == idle);
    assign                              wready                      = (state == idle);
    assign                              wmask                       = 2**awsize; //AXI4协议规定：2的awsize次方
    assign                              bresp                       = 0;
    assign                              bid                         = 0;
    assign                              rid                         = 0;
    assign                              rresp                       = 0;

    always @(*) begin
        case(awsize)
        3'b0:mem_wdata = wdata>>8*awaddr[1:0]; //1字节访问(4份)
        3'b1:mem_wdata = wdata>>16*awaddr[1];  //2字节访问(前半和后半)
        default:mem_wdata = wdata;             //4字节访问(全部)
        endcase
    end
    //总共32位，一字节是8位
    always @(posedge clock or posedge reset) begin
        if(reset)
            state <= idle;
        else begin
            case(state)
            idle:begin
                if(arready & arvalid)
                    state <= addr; //读状态
                else if(awready & awvalid & wready & wvalid)
                    state <= write;//写状态
                else
                    state <= idle; //空闲状态
            end
            addr: state <= idle;
            write: state <= idle;
            default:state <= idle;
            endcase
        end
    end
    //确保回到空闲状态
    always @(posedge clock or posedge reset) begin //读操作逻辑
        if(reset)begin
            rlast <= 1'b0;
            rvalid <= 1'b0;
            rdata <= 0;
        end
        if(state == idle & arready & arvalid)begin
            rlast <= 1'b1;                 //读最后一个标示
            rvalid <= 1'b1;                //读数据有效
            rdata <= npc_pmem_read(araddr);//DPI-C读取内存
        end
        else begin
            rlast <= 1'b0;
            rvalid <= 1'b0;
            rdata <= 0;
        end
    end
    always @(posedge clock or posedge reset) begin //写操作逻辑
        if(reset)begin
            bvalid <= 0;
        end
        else if(state == idle & awready & awvalid & wready & wvalid)begin
            bvalid <= 1'b1;                        //写数据有效
            npc_pmem_write(awaddr,mem_wdata,wmask);//DPI-C写入内存
        end
        else
            bvalid <= 1'b0;
    end




                                                                   
endmodule
