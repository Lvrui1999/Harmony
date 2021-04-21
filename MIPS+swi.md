[TOC]





# 基本信息

1. 字长为$32$，所有寄存器和指令位数都为$32$位。

2. 有$32$个通用寄存器，用$5$位二进制串编码。

3. 特殊寄存器

   | 符号         | 功能                           |
   | ------------ | ------------------------------ |
   | `IR`         | 当前正在执行的指令             |
   | `PC`         | 下一条要执行的指令的地址       |
   | `PTBR`       | 页表基址寄存器                 |
   | `BadVAddr`   | 存储引发内存错误的虚拟内存地址 |
   | `Count`      | 时钟计数                       |
   | `Compare`    | 周期性硬件终端触发条件         |
   | `Status`     | 处理器状态控制                 |
   | `Cause`      | 处理器异常原因                 |
   | `EPC`        | 异常发生时的系统指令地址       |
   | `WatchLo/Hi` | 内存读写硬断点                 |

4. 支持的内存寻址空间为$2^{32}$字节。

5. 支持页式虚拟存储，页大小为$2^{20}$字节。

6. 指令的编码有2种格式

   |      | 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0 |
   | ---- | ------ | ------ | ------ | ------ | ----- | ---- |
   | R    | op     | rs     | rt     | rd     | sa    | func |
   | I    | op     | rs     | rt     | imm    |       |      |
   | J    | op     | rs     | imm    |        |       |      |
   

## 符号约定

| 符号   | 含义                                                         |
| ------ | ------------------------------------------------------------ |
| a.b    | 将a与b的值当作字符串拼接得到的值。比如，0.1表示01。          |
| a(.n)  | 将n份a的值当作字符串拼接得到的值。比如，0(.5)表示00000。     |
| a[u:l] | 表示将a的值看成二进制串，取出从高到低第u位到第l位所包含的子串。比如1001[3:1]表示100。 |
| a[i]   | 表示将a的值看成二进制串，取出从高到低第i位的值。比如1001[3]表示1。 |

## 指令中用到的子过程

### AdvancePC()

将`PC`指向的指令加载到`IR`，并且将`PC+4`。

```flow
st=>start: AdvancePC()
cp=>operation: LW IR, (0)PC 
ad=>operation: ADD PC, PC, 4
st->cp->ad
```

### Exception(e)

```flow
st=>start: Exception(e)
p=>operation: Print(e)
st->p
```

### LoadMemory

读取指定位置的内存。

```flow
st=>start: rs ← LoadMemory(vAddr)
cmp=>condition: vAddr[31:20] = PTBR
err=>operation: 将第vAddr[31:20]页的数据加载进内存
PTBR ← vAddr[31:20]
ld=>operation: 将当前页中vAddr[19:0]处存储的字加载到rs中
st->cmp
cmp(yes)->ld
cmp(no)->err->ld
```



### StoreMemory

向指定位置的内存写入数据。

```flow
st=>start: rs ← StoreMemory(rs,vAddr)
cmp=>condition: vAddr[31:20] = PTBR
err=>operation: 将第vAddr[31:20]页的数据加载进内存
PTBR ← vAddr[31:20]
ld=>operation: 将rs存储的字加载到当前页中vAddr[19:0]处
st->cmp
cmp(yes)->ld
cmp(no)->err->ld
```

### ExceptionHandler()

跳转到异常处理程序运行。

## ADD

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100000 |

### 格式
`ADD rd, rs, rt`

### 作用
1. 将`rs`,`rt`的值相加。如果结果产生了溢出，则`rd`的值不会被改变，程序会抛出异常。
2. 否则，上述计算得到的结果会被写入`rd`。
### 流程
```flow
st=>start: ADD rd, rs, rt
exc=>operation: Exeption(IntegerOverflowErr)
ctemp=>operation: temp ← rs[31].rs[31...0]+rt[31].rt[31...0]
cmpflag=>condition: temp[32] = temp[31]
copy=>operation: rd ← temp
done=>subroutine: AdvancePC()
st->ctemp
ctemp->cmpflag
cmpflag(yes)->copy
copy->done
cmpflag(no)->exc
```

## ADDU

### 编码

| 31..26 |25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100001 |

### 格式
`ADDU rd, rs, rt`
### 作用
将`rs`,`rt`的值相加，并把结果写入`rd`。
### 流程
```flow
st=>start: ADDU rd, rs, rt
ctemp=>operation: temp ← rs+rt
rd ← temp
done=>subroutine: AdvancePC()
st->ctemp
ctemp->done
```

## SUB

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100010 |

### 格式
`SUB rd, rs, rt`

### 作用
1. 计算`rs`减去`rt`的值。如果结果产生了溢出，则`rd`的值不会被改变，程序会抛出异常。
2. 否则，上述计算得到的结果会被写入`rd`。
### 流程
```flow
st=>start: SUB rd, rs, rt
exc=>operation: Exeption(IntegerOverflowErr)
ctemp=>operation: 计算temp ← rs[31].rs[31...0]-rt[31].rt[31...0]
cmpflag=>condition: temp[32] = temp[31]
done=>subroutine: AdvancePC()
copy=>operation: rd ← temp

st->ctemp
ctemp->cmpflag
cmpflag(yes)->copy
copy->done
cmpflag(no)->exc
```

## SUBU

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100011 |

### 格式
`SUBU rd, rs, rt`
### 作用
将`rs`,`rt`的值相减，并把结果写入`rd`。
### 流程
```flow
st=>start: SUBU rd, rs, rt
ctemp=>operation: temp ← rs-rt
rd ← temp
done=>subroutine: AdvancePC()
st->ctemp
ctemp->done
```

## MUL

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 011100 | rs     | rt     | rd     | 00000 | 000010 |

### 格式
`MULU rd, rs, rt`

### 作用
1. 将`rs`,`rt`的值相乘。如果结果产生了溢出，则`rd`的值不会被改变，程序会抛出异常。
2. 否则，上述计算得到的结果会被写入`rd`。
### 流程
```flow
st=>start: MUL rd, rs, rt
exc=>operation: Exeption(IntegerOverflowErr)
ctemp=>operation: temp ← rs * rt
cmpflag=>condition: rs = 0 or temp / x = y
copy=>operation: rd ← temp
done=>subroutine: AdvancePC()
st->ctemp
ctemp->cmpflag
cmpflag(yes)->copy
copy->done
cmpflag(no)->exc
```

## MULU

### 编码

| 31..26 |25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 011001 |

### 格式
`MULU rd, rs, rt`
### 作用
将`rs`,`rt`的值相乘，并把结果写入`rd`。
### 流程
```flow
st=>start: MULU rd, rs, rt
ctemp=>operation: temp ← rs * rt
rd ← temp
done=>subroutine: AdvancePC()
st->ctemp
ctemp->done
```

## DIV

### 编码

| 31..26 |25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 011010 |

### 格式
`DIV rd, rs, rt`
### 作用
将用`rt`的值整除`rs`，并把结果写入`rd`。
### 流程
```flow
st=>start: DIV rd, rs, rt
cond=>condition: rt != 0
ctemp=>operation: temp ← rs / rt
rd ← temp
exp=>operation: Execption(IntegerDivisionByZeroErr)

done=>subroutine: AdvancePC()
st->cond
cond(yes)->ctemp->done
cond(no)->exp
```

## AND

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100100 |

### 格式
`AND rd, rs, rt`

### 作用
将`rs`,`rt`按位与的结果写入`rd`。
### 流程
```flow
st=>start: AND rd, rs, rt
ctemp=>operation: rd ← rs and rt
done=>subroutine: AdvancePC()
st->ctemp
ctemp->done
```

## OR

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100101 |

### 格式
`OR rd, rs, rt`
### 作用
将`rs`,`rt`按位或的结果写入`rd`。
### 流程
```flow
st=>start: OR rd, rs, rt
ctemp=>operation: rd ← rs or rt
done=>subroutine: AdvancePC()
st->ctemp
ctemp->done
```

## XOR

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100110 |

### 格式
`XOR rd, rs, rt`
### 作用
将`rs`,`rt`按位异或的结果写入`rd`。
### 流程
```flow
st=>start: XOR rd, rs, rt
ctemp=>operation: rd ← rs xor rt
done=>subroutine: AdvancePC()
st->ctemp
ctemp->done
```

## NOR

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 100111 |

### 格式
`XOR rd, rs, rt`
### 作用
将`rs`,`rt`按位或非的结果写入`rd`。
### 流程
```flow
st=>start: NOR rd, rs, rt
ctemp=>operation: rd ← rs nor rt
done=>subroutine: AdvancePC()
st->ctemp
ctemp->done
```

## SLT

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 101010 |

### 格式
`SLT rd, rs, rt`
### 作用
将`rs`和`rt`中的值按照有符号整数大小比较的规则进行比较，如果`rs < rt`，则将`1`写入`rd`，否则将`0`写入`rd`。
### 流程
```flow
st=>start: SLT rd, rs, rt
cmp=>condition: rs < rt
done=>subroutine: AdvancePC()
cop1=>operation: rd ← 0(.31).1
cop0=>operation: rd ← 0(.32)
st->cmp
cmp(yes)->cop1
cop1->done
cmp(no)->cop0
cop0->done
```

## SLTU

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | rt     | rd     | 00000 | 101011 |

### 格式
`SLTU rd, rs, rt`
### 作用
将`rs`和`rt`中的值按照无符号整数大小比较的规则进行比较，如果`rs < rt`，则将`1`写入`rd`，否则将`0`写入`rd`。
### 流程
```flow
st=>start: SLT rd, rs, rt
cmp=>condition: 0.rs < 0.rt
done=>subroutine: AdvancePC()
cop1=>operation: rd ← 0(.31).1
cop0=>operation: rd ← 0(.32)
st->cmp
cmp(yes)->cop1
cop1->done
cmp(no)->cop0
cop0->done
```

## SLL

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | 00000  | rt     | rd     | sa    | 000000 |

### 格式
`SLL rd, rt, sa`

### 作用
将`rt`中的值左移`sa`位的结果写入`rd`。
### 流程
```flow
st=>start: SLL rd, rt, sa
done=>subroutine: AdvancePC()
op=>operation: s ← sa
temp ← rt[31-s:0].0(.s)
rd ← temp
st->op
op->done
```

## SRL

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | 00000  | rt     | rd     | sa    | 000010 |

### 格式
`SRL rd, rt, sa`
### 作用
将`rt`中的值逻辑右移`sa`位的结果写入`rd`。
### 流程
```flow
st=>start: SRL rd, rt, sa
done=>subroutine: AdvancePC()
op=>operation: s ← sa
temp ← 0(.s).rt[31:s]
rd ← temp
st->op
op->done
```

## SRA

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | 00000  | rt     | rd     | sa    | 000011 |

### 格式
`SRA rd, rt, sa`
### 作用
将`rt`中的值算术右移`sa`位的结果写入`rd`。
### 流程
```flow
st=>start: SRA rd, rt, sa
done=>subroutine: AdvancePC()
op=>operation: s ← sa
temp ← rt[31](.s).rt[31:s]
rd ← temp
st->op
op->done
```

## SLLV

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | 00000  | rt     | rd     | sa    | 000100 |

### 格式
`SLLV rd, rt, rs`
### 作用
将`rt`中的值左移`rs`位的结果写入`rd`。
### 流程
```flow
st=>start: SLLV rd, rt, rs
done=>subroutine: AdvancePC()
op=>operation: s ← rs[4:0]
temp ← rt[31-s:0]0(.s)
rd ← temp
st->op
op->done
```

## SRLV

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | 00000  | rt     | rd     | sa    | 000110 |

### 格式
`SRLV rd, rt, rs`
### 作用
将`rt`中的值逻辑右移`rs`位的结果写入`rd`。
### 流程
```flow
st=>start: SRLV rd, rt, rs
done=>subroutine: AdvancePC()
op=>operation: s ← rs[4:0]
temp ← 0(.s).rt[31:s]
rd ← temp
st->op
op->done
```

## SRAV

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | 00000  | rt     | rd     | sa    | 000111 |

### 格式
`SRAV rd, rt, rs`
### 作用
将`rt`中的值算术右移`rs`位的结果写入`rd`。
### 流程
```flow
st=>start: SRLV rd, rt, rs
done=>subroutine: AdvancePC()
op=>operation: s ← rs[4:0]
temp ← rt[31](.s).rt[31:s]
rd ← temp
st->op
op->done
```

## JR

### 编码

| 31..26 | 25..21 | 20..16 | 15..11 | 10..6 | 5..0   |
| ------ | ------ | ------ | ------ | ----- | ------ |
| 000000 | rs     | 00000  | 00000  | 00000 | 001000 |

### 格式
`JR rs`
### 作用
如果`rs`中存储的地址是$4$的倍数，就将`rs`中存储的地址写入`PC`，否则产生地址格式错误的异常。
### 流程
```flow
st=>start: JR rs
e=>end:  <- temp
ee=>end: ISAMode <- temp0
mov=>operation: PC <- temp前31位加0
cal=>operation: temp ← rs
panduan=>condition: temp[1] = 0
temp[0] = 0
exc=>operation: Exception(AddressFormatErr)
cop=>operation: PC ← temp
done=>subroutine: AdvancePC()

st->cal->panduan
panduan(yes)->cop->done
panduan(no)->exc
```

## ADDI

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ------ |
| 001000 | rs     | rt     | imm    |

### 格式
`ADDI rt, rs, imm`
### 作用
1. 将16位立即数`imm`与`rt`中存储的32位整数进行加法。如果结果产生了溢出，则`rd`的值不会被改变，程序会抛出异常。
2. 否则，上述计算得到的结果会被写入`rd`。
### 流程

```flow
st=>start: ADDI rt, rs, imm
e=>subroutine: AdvancePC()
temp=>operation: temp ← rs[31].rs[31:0] + imm[15](.16).imm
ifover=>condition: temp[32] = temp[31]
err_overflow=>operation: Exception(IntergerOverflowErr)
save=>operation: rt ← temp


st->temp->ifover
ifover(no)->err_overflow->e
ifover(yes)->save->e
```

## ADDIU

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 001001 | rs     | rt     | imm   |

### 格式

`ADDU rt, rs, imm`

### 作用

将一个16位有符号立即数`imm`与`rs`中的32位整数相加，并把结果写入`rt`。

### 流程

```flow
st=>start: ADDU rt, rs, imm
e=>subroutine: AdvancePC()
temp=>operation: temp ← rs[31].rs[31:0] + imm[15](.16).imm
save=>operation: rt  ← temp


st->temp->save->e
```

## INC
### 编码

| 31..26 | 25..21 | 20..0 |
| ------ | ------ | ------ | ----- |
| 110000 | rs     | 000000000000000000   |

### 格式

`INC rs`

### 作用
将`rs`中的值+1。

```flow
st=>start: INC rs
push=>operation: ADDI rs, rs, 1
e=>subroutine: AdvancePC()
st->push->e
```

## DEC
### 编码

| 31..26 | 25..21 | 20..0 |
| ------ | ------ | ------ | ----- |
| 110001 | rs     | 000000000000000000   |

### 格式

`DEC rs`

### 作用
将`rs`中的值-1。

```flow
st=>start: INC rs
push=>operation: ADDI rs, rs, 0xFFFF
e=>subroutine: AdvancePC()
st->push->e
```

## ANDI

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 001100 | rs     | rt     | imm   |

### 格式

`ANDU rt, rs, imm`。

### 作用

将一个16位左侧零扩展立即数`imm`与`rs`中的32位整数进行按位与，结果将存至`rt`。

### 流程

```flow
st=>start: ANDU rt, rs, imm
e=>subroutine: AdvancePC()
temp=>operation: rt ← rs and 0(.16).imm


st->temp->e
```

## ORI

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 001101 | rs     | rt     | imm   |

### 格式

`ORI rt, rs, imm`

### 作用

将一个16位左侧零扩展立即数`imm`与`rs`中的32位整数进行按位或，结果将存至`rt`。

### 流程

```flow
st=>start: ORI rt, rs, imm
e=>subroutine: AdvancePC()
temp=>operation: rt ← rs or 0(.16).imm


st->temp->e
```

## XORI

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 001110 | rs     | rt     | imm   |

### 格式

`XORI rt, rs, imm`

### 作用

将一个16位左侧零扩展立即数`imm`与`rs`中的32位整数进行按位异或，结果将存至`rt`。

### 流程

```flow
st=>start: XORI rt, rs, imm
e=>subroutine: AdvancePC()
temp=>operation: rt ← rs or 0(.16).imm


st->temp->e
```

## LUI

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 001111 | 00000  | rt     | imm   |

### 格式

`LUI rt, imm`

### 作用

将一个16位立即数`imm`左移16位并与16位零连接，将结果将存至rt。

### 流程

```flow
st=>start: LUI rt, imm
e=>subroutine: AdvancePC()
temp=>operation: rt ← imm.0(.16)


st->temp->e
```

## LW

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 100011 | rb     | rt     | imm   |

### 格式

`LW rt, imm(rb)`

### 作用

从寄存器`rb`读取基址与偏移`imm`相加后得到有效地址，并从指定内存地址读取32位字将结果存入`rt`。

### 流程

```flow
st=>start: LW rt, imm(rb)
e=>subroutine: AdvancePC()
addr=>operation: vAddr ← imm[15](.16).imm + rb
trans=>operation: (pAddr,CCA)  ← AddressTranslation(vAddr,DATA,LOAD)
load=>operation:  memword ← LoadMemory(vAddr)
save=>operation: rt ← memword


st->addr->load->save->e
```

## SW

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 101011 | rb     | rt     | imm   |

### 格式

`SW rt, rb(imm)`

### 作用

从寄存器`rb`读取基址与偏移`imm`相加后得到有效地址，将寄存器`rt`的值存入内存的该地址位置。

### 流程

```flow
st=>start: SW rt, rb(imm)
e=>subroutine: AdvancePC()
addr=>operation: vAddr ← imm[15](.16).imm + rb
trans=>operation: (pAddr,CCA) ← AddressTranslation(vAddr,DATA,STORE)
load=>operation: dataword ← rt
save=>operation: StoreMemory(dataword,vAddr)


st->addr->load->save->e
```

## MOV


### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 111111 | rs     | rt     | 0000000000000000|

### 格式

`MOV rs, rt`

### 作用

将`rs`中的值写入`rt`。

### 流程

```flow
st=>start: MOV rs, rt
e=>subroutine: AdvancePC()
addr=>operation: rt ← rs

st->addr->e
```

## XCHG


### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 111110 | rs     | rt     | 0000000000000000|

### 格式

`XCHG rs, rt`

### 作用

交换`rs`，`rt`的值。

### 流程

```flow
st=>start: XCHG rs, rt
e=>subroutine: AdvancePC()
addr=>operation: temp ← rt
rt ← rs
rs ← temp
st->addr->e
```


## BEQ

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 000100 | rs     | rt     | imm   |

### 格式

`BEQ rs, rt, imm`

### 作用

比较`rs`和`rt`的值。当它们的值相等时，将16位`imm`左移2位后与当前的下一条指令地址相加来获得下一条指令的地址。

### 流程

```flow
st=>start: BEQ rs, rt, imm
e=>subroutine: AdvancePC()
test=>operation: cond ← (rs = rt)
if=>condition: rs = rt
branch=>operation: PC = PC + imm[15](.14)imm.00

st->if
if(yes)->branch->e
if(no)->e
```

## BNE

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 000101 | rs     | rt     | imm   |

### 格式

`BNE rs, rt, imm`

### 作用

比较`rs`和`rt`的值。当它们的值不等时，将16位`imm`左移2位后与当前的下一条指令地址相加来获得下一条指令的地址。

### 流程

```flow
st=>start: BEQ rs, rt, imm
e=>subroutine: AdvancePC()
test=>operation: cond ← (rs = rt)
if=>condition: rs != rt
branch=>operation: PC = PC + imm[15](.14)imm.00

st->if
if(yes)->branch->e
if(no)->e
```

## SLTI

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 001010 | rs     | rt     | imm   |

### 格式

`SLTI rs, rt, imm`

### 作用

比较寄存器`rs`和16位有符号立即数`imm`的大小，如果`rs`小于`imm`，则向`rt`写入1，否则写入0。

### 流程

```flow
st=>start: SLTI rs, rt, imm
e=>subroutine: AdvancePC()
if=>condition: rs < imm[15](.16).imm
ok=>operation: rs = 0(.31)1
no=>operation: rs = 0(.32)

st->if
if(yes)->ok->e
if(no)->no->e
```

## SLTIU

### 编码

| 31..26 | 25..21 | 20..16 | 15..0 |
| ------ | ------ | ------ | ----- |
| 001011 | rs     | rt     | imm   |

### 格式

`SLTIU rs, rt, imm`

### 作用

以无符号规则比较寄存器`rs`和16位立即数`imm`的大小，如果`rs`小于立即数`imm`，则向`rt`写入1，否则写入0。

```flow
st=>start: SLTIU rs, rt, imm
e=>subroutine: AdvancePC()
if=>condition: 0.rs < 0.imm[15](.16).imm
ok=>operation: rs ← 0(.31)1
no=>operation: rs ← 0(.32)

st->if
if(yes)->ok->e
if(no)->no->e
```

## PUSH
### 编码

| 31..26 | 25..21 | 20..0 |
| ------ | ------ | ------ | ----- |
| 100000 | rs     | 000000000000000000   |

### 格式

`PUSH rs`

### 作用
将`rs`中的值压栈。

```flow
st=>start: PUSH rs
push=>operation: DEC sp
SW rs, (0)sp
e=>subroutine: AdvancePC()
st->push->e
```

## POP
### 编码

| 31..26 | 25..21 | 20..0 |
| ------ | ------ | ------ | ----- |
| 100001 | rs     | 000000000000000000   |

### 格式

`POP rs`

### 作用
将栈顶的值写入`rs`并弹出。

```flow
st=>start: POP rs
push=>operation: LW rs, (0)sp
INC sp
e=>subroutine: AdvancePC()
st->push->e
```



## SWI

### 编码

| 31..26 | 25..21 | 20..0              |
| ------ | ------ | ------------------ |
| 010010 | 00000  | 000000000000000000 |

### 格式

`SWI`

### 作用

触发软中断。

```flow
st=>start: PUSH rs
push=>operation: EPC = PC
Cause = SYSCALL
jp=>subroutine: ExceptionHandler() 
e=>subroutine: AdvancePC()
st->push->jp->e
```

## MFC0

### 编码

| 31..26 | 25..21 | 20..16 | 15..0            |
| ------ | ------ | ------ | ---------------- |
| 101011 | rd     | rt     | 0000000000000000 |

### 格式

`mfc0 rt, rd`

### 作用

特权指令。从特殊寄存器`rd`读取内容存入通用寄存器`rd`

## MTC0

### 编码

| 31..26 | 25..21 | 20..16 | 15..0            |
| ------ | ------ | ------ | ---------------- |
| 101011 | rd     | rt     | 0000000000000000 |

### 格式

`mtc0 rt, rd`

### 作用

特权指令。从通用寄存器`rd`读取内容存入特殊寄存器`rd`