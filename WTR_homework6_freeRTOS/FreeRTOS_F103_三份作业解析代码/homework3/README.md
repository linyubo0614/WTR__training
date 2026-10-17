# 作业三

复制本文件夹 UserCode 的3个文件到工程，并加入编译。
USART1 PA9/PA10，115200、8N1，必须启用 USART1 中断，抢占优先级5。
PB0 配置输入上拉，按钮接地；PC13配置推挽输出，默认高电平。

上电是 RAW。按下并松开按钮切换 FUSED，再按切回 RAW。按住只切换一次。
软件定时器周期100ms和150ms，传感器队列每条消息严格为9字节。
UART状态Watch变量：g_uart_state，0等待数据、1发送、2错误。
串口事件文字：WAIT_DATA、SENDING、ERROR。事件报告自身也需要占用UART。

观察丢弃规则：HW3_STALE_DEMO=1；正常运行恢复0。
观察错误状态：HW3_UART_TIMEOUT_DEMO=1；正常运行恢复0。
连接断开不会使STM32的TX自动检测到电脑离线，用超时注入测试更明确。

注释代码已经通过电脑端语法检查和融合逻辑测试，但不是完整HAL/FreeRTOS工程。
请从公共ioc生成芯片工程后添加UserCode，再板上编译、下载与验证。
