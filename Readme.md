# Project_HTTP



**一个HTTP服务器的简单实现**

`./make.sh`运行，生成output目录，进入output目录`./server`即可运行



## 结果展示

#### 1.浏览器测试

* 直接输入IP和端口号，浏览器默认加上/，进入首页

  <img src=".\image\6.jpg" alt="6" style="zoom:30%;" />

  首页有一些文本，以及一个表单
  输入100和50，点击submit，之后进入test_cgi可执行程序进行处理，返回如下结果

  <img src=".\image\7.jpg" alt="7" style="zoom:30%;" />

* 如果输入100和0

  <img src=".\image\8.jpg" alt="8" style="zoom:30%;" />

  直接发生除0错误，返回400

  <img src=".\image\9.jpg" alt="9" style="zoom:30%;" />

* 访问在根目录下的a目录，即/a

  <img src=".\image\10.jpg" alt="10" style="zoom:30%;" />

  依然可以显示首页，并且可以进行正常输入和运行

  <img src=".\image\11.jpg" alt="11" style="zoom:30%;" />

* 访问一个不存在的网页，返回404

  <img src=".\image\12.jpg" alt="12" style="zoom:30%;" />

#### 2.telnet测试

<img src="./image/13.jpg" alt="13" style="zoom:30%;" />

![14](./image/14.jpg)

![15](./image/15.jpg)

可以看到，get和post方法都可以正确发送请求和响应

#### 3.Postman测试

* GET方法

  ![16](./image/16.jpg)

* POST方法

  ![17](./image/17.jpg)
