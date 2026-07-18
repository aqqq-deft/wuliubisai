

void handle_uart1() {
    static unsigned int index=0, time1=0, pwm1=0, i, u_i = 0;
    int left_pwm, right_pwm;
    unsigned int len;
    uart1_receive_buf = "";

    if(Serial.available()) {
        while(Serial.available() > 0)  {        //如果串口有数据
            uart1_receive_buf += char(Serial.read());
            //delay(1);
        }
    } else if(mySerial.available()) {
        while(mySerial.available() > 0) {        //判断串口缓存区是否有数据
            uart1_receive_buf += char(mySerial.read());//读取串口数据，并保存到变量comdata中
            delay(1);                              //延时一段时间
        }
    }
    
    if(uart1_receive_buf.length() > 100) {
       uart1_receive_buf = "";
    } else if(uart1_receive_buf.length() > 1) {     //如果串口有数据
        time_max = 0;
      //  Serial.println(uart1_receive_buf);
        if(!(uart1_receive_buf[uart1_receive_buf.length()-1]=='!' || uart1_receive_buf[uart1_receive_buf.length()-1]=='}') )return;
        if( (uart1_receive_buf[0] == '#'&&  uart1_receive_buf[uart1_receive_buf.length()-1]=='!') || (uart1_receive_buf[0] == '{' && uart1_receive_buf[uart1_receive_buf.length()-1]=='}') ) {   //解析以“#”或者以“{”开头的指令
            len = uart1_receive_buf.length();  //获取串口接收数据的长度
            index=0; pwm1=0; time1=0;           //3个参数初始化
            for(i = 0; i < len; i++) {              //如果数据没有接收完
                if(uart1_receive_buf[i] == '#') {         //判断是否为起始符“#”
                    i++;                                  //下一个字符
                    while(uart1_receive_buf[i] != 'P' && i < len) {  //判断是否为#后面的数字检测完
                        index = index*10 + uart1_receive_buf[i] - '0';  //记录P之前的数字
                        i++;
                    }
                    if(i>len)return;
                    i--;                          //因为上面i多自增一次，所以要减去1个
                } else if(uart1_receive_buf[i] == 'P') {  //检测是否为“P”
                    i++;
                    while(uart1_receive_buf[i] != 'T' && i < len) {  //P之后的数字检测并保存
                        pwm1 = pwm1*10 + uart1_receive_buf[i] - '0';
                        i++;
                    }
                    if(i>len)return;
                    i--;
                } else if(uart1_receive_buf[i] == 'T') {  //判断是否为“T”
                    i++;
                    while(uart1_receive_buf[i] != '!' && i < len) {
                        time1 = time1*10 + uart1_receive_buf[i] - '0';    //将T后面的数字保存
                        i++;
                    }
                    if(i>len)return;
                    
                    if((index >= DJ_NUM)
                    ||(pwm1 > 2500)
                    ||(pwm1<500)) {             //如果舵机号 和 pwm1数值超出约定值，则跳出不处理 
                        break;
                    }
                    //检测完后赋值
                    myservo[index].attach(duoji_pin[index]);
                    duoji_doing_flag[index] = 0;
                    duoji_doing[index].aim = pwm1;         //舵机pwm1赋值
                    duoji_doing[index].time1 = time1;      //舵机执行时间赋值
                    float pos_err = duoji_doing[index].aim - duoji_doing[index].cur;
                    duoji_doing[index].inc = (pos_err*1.000)/(duoji_doing[index].time1/20.000); //根据时间计算舵机pwm1增量
                    time_max = max(time_max, time1);
            #if 0 //调试的时候读取数据用
                    Serial.print("index = ");
                    Serial.println(index);
                    Serial.print("pwm1 = ");
                    Serial.println( duoji_doing[index].aim);
                    Serial.print("time = ");
                    Serial.println(duoji_doing[index].time1);
                    Serial.print("time_max = ");
                    Serial.println(time_max);
            #endif       
                index = pwm1 = time1 = 0; 
                }
                
            }
        } else if(uart1_receive_buf[0] == '$' && (uart1_receive_buf[1] == 'D') && (uart1_receive_buf[2] == 'S') 
              && (uart1_receive_buf[3] == 'T') && (uart1_receive_buf[4] == '!')) { //解析以"$"开头的指令
               for(byte i = 0; i < DJ_NUM; i++) {
                    duoji_doing[i].aim =  (int)duoji_doing[i].cur;
                    myservo[i].writeMicroseconds(duoji_doing[i].aim); 
               } 
                                                                                                        
        } else if(uart1_receive_buf[0] == '$' && (uart1_receive_buf[1] == 'D') && (uart1_receive_buf[2] == 'S') 
              && (uart1_receive_buf[3] == 'T') && (uart1_receive_buf[4] == ':') && (uart1_receive_buf[6] == '!')) { //解析以"$"开头的指令
               i = uart1_receive_buf[5] - '0';
               if( i < DJ_NUM) {
                  duoji_doing[i].aim =  (int)duoji_doing[i].cur;
                  if(duoji_doing_flag[i] ==0) {
                      myservo[i].attach(duoji_pin[i]);
                      delay(20);
                      myservo[i].writeMicroseconds(duoji_doing[i].aim); 
                      delay(20);
                      myservo[i].detach();
                  }
                  
               }
                                                                                                        
        } else if(uart1_receive_buf[0] == '$' && (uart1_receive_buf[1] == 'D') && (uart1_receive_buf[2] == 'J') 
              && (uart1_receive_buf[3] == 'R') && (uart1_receive_buf[4] == '!')) { //解析以"$"开头的指令
                for(i=0;i<DJ_NUM;i++){
                  myservo[i].attach(duoji_pin[i]);
               }
               delay(50);
               for(i=0;i<DJ_NUM;i++){
                  duoji_doing[i].cur = 1500;
                  duoji_doing[i].aim =  1500;
                  myservo[i].writeMicroseconds(duoji_doing[i].aim); 
                  
               }
               delay(50);
              for(i=0;i<DJ_NUM;i++){
                myservo[i].detach();
              }
                                                                                                        
        } else if(uart1_receive_buf[0] == '$' && (uart1_receive_buf[1] == '!') ) { //解析以"$"开头的指令
               Serial.print("$!");                                                                                                            
        }
        last_time =  millis();
       // Serial.print(uart1_receive_buf); 
        uart1_receive_buf = "";
    }  
}
