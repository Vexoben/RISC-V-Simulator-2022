## 用C++实现的一个RISC-V模拟器

- sequential：顺序执行

- tomasulo：tomasulo算法，尝试了分支预测

### 分支预测的正确率

由于 JAL 指令和 JALR 指令是永远预测正确和永远预测错误，不计入正确率的计算。这里只统计所有的 branch 类型指令

#### 预测永远跳转

| 测试点         | 总预测数 | 预测正确数 | 预测正确率 | 总时钟周期 |
| -------------- | -------- | ---------- | ---------- | ---------- |
|array_test1|22|10|0.454545|285|
|array_test2|26|13|0.5|315|
|basicopt1|155139|63642|0.410226|1068921|
|bulgarian|71493|35289|0.493601|990137|
|expr|111|69|0.621622|879|
| gcd            |          |            |            |            |
| hanoi          |          |            |            |            |
| lvalue2        |          |            |            |            |
| magic          |          |            |            |            |
| manyarguments  |          |            |            |            |
| multiarray     |          |            |            |            |
| naive          |          |            |            |            |
| pi             |          |            |            |            |
| qsort          |          |            |            |            |
| queens         |          |            |            |            |
| statement_test |          |            |            |            |
| superloop      |          |            |            |            |
| tak            |          |            |            |            |

