// https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/#_compressed_instruction_formats about 1/3 the way
// down. https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
// This adds -C extension support to mini-rv32ima.

#include <stdio.h>

#define MINIRV32_HANDLE_OTHER_OPCODE																				   \
	default:																										   \
	{																												  \
		int cimm		 = ((((ir >> 2) & 0x1f)) | (((ir >> 12) & 1) << 5));										   \
		uint32_t cimmext = (cimm & 0x20) ? (cimm | 0xffffffc0) : cimm;												 \
		switch (ir & 3)																								\
		{																											  \
			case 0b00:																								 \
			{																										  \
				ir &= 0xffff;																						  \
				pc -= 2;																							   \
				uint32_t uimm = (((ir >> 5) & 1) << 6) | (((ir >> 6) & 1) << 2) | (((ir >> 10) & 7) << 3);			 \
				switch (ir >> 13)																					  \
				{																									  \
					case 0b000: /*c.addi4spn ADD Imm * 4 + SP (I think this is right?  Maybe)?*/				   \
						cimm = (((ir >> 5) & 1) << 3) | (((ir >> 6) & 1) << 2) | (((ir >> 7) & 0xf) << 6)			  \
							   | (((ir >> 11) & 3) << 4);															  \
						cimmext = (cimm & 0x200) ? (cimm | 0xfffffc00) : cimm;										 \
						rdid	= ((ir >> 2) & 7) + 8;																 \
						/*printf( "c.addi4spn %08x %d / %d @ %08x\n", REG( 2 ), cimmext, rdid, debugpc );*/							\
						rval = REG(2) + cimmext;																	   \
						break;																						 \
					case 0b010: { /*c.lw*/																			   \
						printf( "L %08x -> %08x/%08x\n", pc, REG(((ir>>7)&7)+8), uimm );						   \
                        uint32_t addr = (REG(((ir >> 7) & 7) + 8) + uimm) - MINIRV32_RAM_IMAGE_OFFSET; \
                        if (addr >= MINI_RV32_RAM_SIZE-3) { \
                            addr += MINIRV32_RAM_IMAGE_OFFSET; \
                            if (MINIRV32_MMIO_RANGE(addr)) { \
                                printf("extram load %08x\n", addr); \
                                rval = _uvm32_extramLoad(userdata, addr, 2); /* 2 = word */ \
                            } else { \
                                printf("BAD LOAD ADDR %08x\n", addr); \
							    trap = (5+1); \
							    /*rval = rsval; ?? */ \
                            } \
                        } else { \
                            rval = MINIRV32_LOAD4(addr); \
                        } \
						rdid = ((ir >> 2) & 7) + 8;																	\
						} break;																						 \
					case 0b110: { /*c.sw*/																			   \
						printf( "S %08x -> %08x/%08x\n", pc, REG(((ir>>7)&7)+8), uimm );	   				   \
                        uint32_t addr = (REG(((ir >> 7) & 7) + 8) + uimm) - MINIRV32_RAM_IMAGE_OFFSET; \
                        if (addr >= MINI_RV32_RAM_SIZE-3) { \
                            addr += MINIRV32_RAM_IMAGE_OFFSET; \
                            if (MINIRV32_MMIO_RANGE(addr)) { \
                                /*MINIRV32_HANDLE_MEM_STORE_CONTROL(addr - 0x10000000, REG(((ir >> 2) & 7) + 8));*/ \
                                printf("extram store %08x val=%08x\n", addr, REG(((ir >> 2) & 7) + 8)); \
                                _uvm32_extramStore(userdata, addr, REG(((ir >> 2) & 7) + 8), 2); /* 2 = word */ \
                            } else { \
                                printf("BAD STORE ADDR %08x\n", addr); \
                                trap = (5+1); \
                                /*rval = rsval; ?? */ \
                            } \
                        } else { \
						    MINIRV32_STORE4(addr, REG(((ir >> 2) & 7) + 8)); \
                        } \
						rdid = 0;																					  \
						} break;																						 \
					default:																						   \
                        /* printf( "Unknown Opcode at %08x\n", debugpc ); */                                          \
						trap = (2 + 1);																				\
						break;																						 \
				}																									  \
				break;																								 \
			}																										  \
			case 0b01:																								 \
				ir &= 0xffff;																						  \
				pc -= 2;																							   \
				switch (ir >> 13)																					  \
				{																									  \
					case 0b000: /*c.addi*/																			 \
						rval = REG(rdid) + cimmext;																	\
						break;																						 \
					case 0b010: /*c.li*/																			   \
						rval = cimmext;																				\
						break;																						 \
					case 0b011: /*c.lui / ADDI16SP*/																   \
						switch (((ir >> 7) & 0x1f))																	\
						{																							  \
							case 0:																					\
								break;																				 \
							case 2: /*c.addi16sp TODO: Check me.*/													 \
							{																						  \
								cimm = (((ir >> 12) & 1) << 9) | (((ir >> 2) & 1) << 5) | (((ir >> 5) & 1) << 6)	   \
									   | (((ir >> 6) & 1) << 4) | (((ir >> 3) & 3) << 7);							  \
								cimmext = (cimm & 0x200) ? (cimm | 0xfffffc00) : cimm;								 \
								/*printf( "c.addi16sp -> %08x -> %08x\n",  REG(2), cimmext );*/						\
								rval = REG(2) + cimmext;															   \
								break;																				 \
							}																						  \
							default: /*c.lui*/																		 \
								rval = cimm << 12;																	 \
								break;																				 \
						};																							 \
						break;																						 \
					case 0b100: /* MISC-ALU */																		 \
						rdid = (rdid & 7) + 8;																		 \
						switch ((ir >> 10) & 3)																		\
						{																							  \
							case 0: /*c.srli*/																		 \
								rval = REG(rdid) >> cimm;															  \
								break;																				 \
							case 1: /*c.srai*/																		 \
								rval = ((int32_t)REG(rdid)) >> cimm;												   \
								break;																				 \
							case 2: /*c.andi*/																		 \
								rval = REG(rdid) & cimm;															   \
								break;																				 \
							case 3: /*c.other*/																		\
							{																						  \
								uint32_t rs2 = REG((cimm & 7) + 8);													\
								switch (cimm >> 3)																	 \
								{																					  \
									case 0: /* c.sub */																\
										rval = REG(rdid) - (rs2);													  \
										break;																		 \
									case 1: /* c.xor */																\
										rval = REG(rdid) ^ (rs2);													  \
										break;																		 \
									case 2: /* c.or  */																\
										rval = REG(rdid) | (rs2);													  \
										break;																		 \
									case 3: /* c.and */																\
										rval = REG(rdid) & (rs2);													  \
										break;																		 \
									default: /* res   */															   \
                                        /* printf( "Unknown Opcode at %08x\n", debugpc ); */                            \
										trap = (2 + 1);																\
										break;																		 \
								}																					  \
							}																						  \
						}																							  \
						break;																						 \
					case 0b001: /*c.jal + NOTE: Is c.addiw on RV64*/												   \
					case 0b101: /*c.j*/																				\
					{																								  \
						uint32_t limm = (((ir >> 2) & 0x1) << 5) | (((ir >> 3) & 0x7) << 1) | (((ir >> 6) & 0x1) << 7) \
										| (((ir >> 7) & 0x1) << 6) | (((ir >> 8) & 0x1) << 10)						 \
										| (((ir >> 9) & 0x3) << 8) | (((ir >> 11) & 0x1) << 4)						 \
										| (((ir >> 12) & 1) << 11);													\
						if (limm & 0x800)																			  \
							limm |= 0xfffff000;																		\
						if ((ir >> 13) == 0b001)																	   \
						{																							  \
							rdid = 1;																				  \
							rval = pc + 4;																			 \
							/*printf( "jal r1 = %08x\n", rval );*/													 \
						}																							  \
						else																						   \
							rdid = 0;																				  \
						pc = pc + limm - 2;																			\
					}																								  \
					break;																							 \
					case 0b110: /*c.beqz*/																			 \
					case 0b111: /*c.bnez*/																			 \
					{																								  \
						rdid		  = 0;																			 \
						uint32_t limm = (((ir >> 2) & 0x1) << 5) | (((ir >> 3) & 0x3) << 1)							\
										| (((ir >> 10) & 0x3) << 3) | (((ir >> 5) & 0x3) << 6)						 \
										| (((ir >> 12) & 0x1) << 8);												   \
						if (limm & 0x100)																			  \
							limm |= 0xffffff00;																		\
						if ((ir >> 13) == 0b110)																	   \
						{																							  \
							if (REG(((ir >> 7) & 0x7) + 8) == 0)													   \
							{																						  \
								pc = pc + limm - 2;																	\
							}																						  \
						}																							  \
						else																						   \
						{																							  \
							if (REG(((ir >> 7) & 0x7) + 8) != 0)													   \
							{																						  \
								pc = pc + limm - 2;																	\
							}																						  \
						}																							  \
						break;																						 \
					}																								  \
					default:																						   \
                        /* printf( "Unknown Opcode at %08x\n", debugpc ); */                                          \
						trap = (2 + 1);																				\
						break;																						 \
				}																									  \
				break;																								 \
			case 0b10:																								 \
				ir &= 0xffff;																						  \
				pc -= 2;																							   \
				switch (ir >> 13)																					  \
				{																									  \
					case 0b000: /*c.slli*/																			 \
						rval = REG(rdid) << cimm;																	  \
						break;																						 \
					case 0b100: /*c.mv / c.add / c.jr (/c.ret) / c.jalr */											 \
						if ((ir >> 12) & 1)																			\
						{																							  \
							if ((((ir >> 2) & 0x1f) != 0) && rdid != 0)												\
							{																						  \
								/*printf( "Adding r %d / %d\n", (ir>>2)&0x1f, rdid );*/								\
								rval = REG((ir >> 2) & 0x1f) + REG(rdid); /* c.add */								  \
							}																						  \
							else if (rdid != 0)																		\
							{																						  \
								rval = pc + 4;																		 \
								pc   = REG(rdid) - 4; /*c.jr*/														 \
								rdid = 1; /* c.jalr */																 \
							}																						  \
							else																					   \
							{																						  \
                                /* printf( "Unknown Opcode at %08x\n", debugpc ); */                                  \
								trap = (3 + 1);																		\
								break; /* EBREAK 3 = "Breakpoint" */												   \
							}																						  \
						}																							  \
						else																						   \
						{                                                                                         \
							if ((((ir >> 2) & 0x1f) != 0) && rdid != 0)												\
							{																						  \
								/* c.mv */                                                                     \
								rval = REG((ir >> 2) & 0x1f);														  \
							    /* printf( "c.mv %d %d -> %08x @ %08x\n", rdid, (ir >> 2) & 0x1f, rval,pc);  */      	\
							}																						  \
							else if (rdid != 0)																		\
							{																						  \
								pc   = REG(rdid) - 4;																  \
								rdid = 0; /* c.jr */																   \
							}																						  \
							else																					   \
							{																						  \
								/* illegal opcode */																   \
                                /* printf( "Unknown Opcode at %08x\n", debugpc ); */                                  \
								trap = (2 + 1);																		\
								break;																				 \
							}																						  \
						}																							  \
						break;																						 \
					case 0b110: /*c.swsp / c.sw(SP)*/																  \
						rdid = 0;																					  \
						/*printf( "C.SWSP gp=%08x -> REG(2)=%08x + %08x <<< %08x\n", REG(3), REG(2), (((ir>>7)&3)<<6)  \
						 * + (((ir>>9)&0xf)<<2), REG(((ir>>2)&0x1f)) ); */											 \
                         MINIRV32_STORE4((REG(2) + (((ir >> 7) & 3) << 6) + (((ir >> 9) & 0xf) << 2)) - MINIRV32_RAM_IMAGE_OFFSET,   \
										REG(((ir >> 2) & 0x1f)));													  \
						break;																						 \
					case 0b010: /*c.lwsp / c.lw(SP)*/																  \
						/*printf( "C.LWSP gp=%08x -> REG(2)=%08x + %08x <<< %08x\n", REG(3), REG(2), (((ir>>2)&3)<<6)  \
						 * + (((ir>>4)&0x7)<<2) + (((ir>>12)&1)<<5), REG(((ir>>2)&0x1f)) );*/						  \
						rval = MINIRV32_LOAD4((REG(2) + (((ir >> 2) & 3) << 6) + (((ir >> 4) & 0x7) << 2)			   \
											  + (((ir >> 12) & 1) << 5)) - MINIRV32_RAM_IMAGE_OFFSET);				  \
						rdid = ((ir >> 7) & 0x1f);																	 \
						break;																						 \
					default:																						   \
                        /* printf( "Unknown Opcode at %08x\n", debugpc ); */                                          \
						trap = (2 + 1);																				\
						break;																						 \
				}																									  \
				break;																								 \
			default:																								   \
                /* printf( "Unknown Opcode at %08x\n", debugpc ); */                                                 \
				trap = (2 + 1);																						\
				break;																								 \
		}																											  \
	}

