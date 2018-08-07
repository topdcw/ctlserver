// ctlServer.cpp : 定义控制台应用程序的入口点。
//

// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved
//
// This example demonstrates how to handle very large requests without keeping
// them in memory.

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include "tinyxml.h"
#include "mongoose.h"

using namespace std;
int translateXml();

//g++ -fexec-charset=utf-8 ctlServer.cpp mongoose.c tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp -o server
void process_xml(const char *file_name, char* data, long len) //数据的真实长度是 len-1， 最后补了一个字节 0，即：data[len] == 0x00 ，以方便字符串处理。
{
	//打印文件内容（作为 字符串）
	printf("process_xml  : '%s'  :  %ld Bytes\n", file_name, len-1);
	//printf("%s\n", data);

	//判断 文件内容是否为 xml 格式，如果是，则 解析、转换为 opnet 的 网络拓扑 xml 格式文件
	
	//先将data存到文件test.xml里去。
	string path = "test.xml";
	ofstream outfile;
	outfile.open(path, ios::binary);
	if (!outfile)
	{
		cout << "xml文件打开失败!" << endl;
		exit(0);
	}
	outfile << data;
	outfile.close();
	//启用转化程序
	translateXml();
	cout << "操作转换完成！" << endl;

}

void process_hop(const char *file_name, char* data, long len) //数据的真实长度是 len-1， 最后补了一个字节 0，即：data[len] == 0x00 ，以方便字符串处理。
{
	//打印文件内容（作为 字符串）
	printf("process_hop  : '%s'  :  %ld Bytes\n", file_name, len-1);
	//printf("%s\n", data);

	//判断 文件内容是否为 hop 格式，如果是，则 解析 跳频文件

	//将data存到文件***.hop里去。
	string path = "";
	path += file_name;
	ofstream outfile;
	outfile.open(path, ios::binary);
	if (!outfile)
	{
		cout << "hop文件打开失败!" << endl;
		exit(0);
	}
	outfile << data;
	outfile << endl;
	outfile<<"\n";
	outfile.flush();
	outfile.close();
	printf("hop文件接收完毕\n");
}


bool process_cmd(const char *file_name, char* data, long len) //数据的真实长度是 len-1， 最后补了一个字节 0，即：data[len] == 0x00 ，以方便字符串处理。
{
	bool ret = true;
	//打印文件内容（作为 字符串）
//	printf("process_cmd  : '%s'  :  %ld Bytes\n", file_name, len-1);
//	printf("%s\n", data);

	//判断文件内容 是否为 命令字符串，如：  run,  stop,  pause，resume 如果是，则执行相应的动作
	if(strcmp(data, "run")==0)
	{
		printf("[CMD] run\n");
		ret = false;
	}
	if(strcmp(data, "isCMD_run_finished")==0)
	{
		printf("[CMD] isCMD_run_finished\n");
		ret = false;
	}
	else if(strcmp(data, "stop")==0)
	{
		printf("[CMD] stop\n");
	}
	else if(strcmp(data, "pause")==0)
	{
		printf("[CMD] pause\n");
		ret = false;
	}
	else if(strcmp(data, "resume")==0)
	{
		printf("[CMD] resume\n");
	}
	else if( (len-1>strlen("runtime="))&&(memcmp("runtime=",data,strlen("runtime="))==0))// runtime=xxx
	{
		int runtime = atoi(data+strlen("runtime="));
		printf("[CMD] runtime: %d\n", runtime);
	}
	else
	{
		printf("Error: cmd '%s' is invalid\n", data);
	}

	return ret;
}


static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

struct file_writer_data {
  FILE *fp;
  size_t bytes_written;
};



static void handle_upload_xml(struct mg_connection *nc, int ev, void *p) {
  struct file_writer_data *data = (struct file_writer_data *) nc->user_data;
  struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;

  switch (ev) {
    case MG_EV_HTTP_PART_BEGIN: {
      if (data == NULL) {
        data = (struct file_writer_data*)calloc(1, sizeof(struct file_writer_data));
        data->fp = tmpfile();
        data->bytes_written = 0;

        if (data->fp == NULL) {
          mg_printf(nc, "%s",
                    "HTTP/1.1 500 Failed to open a file\r\n"
                    "Content-Length: 0\r\n\r\n");
          nc->flags |= MG_F_SEND_AND_CLOSE;
          free(data);
          return;
        }
        nc->user_data = (void *) data;
      }
      break;
    }
    case MG_EV_HTTP_PART_DATA: {
      if (fwrite(mp->data.p, 1, mp->data.len, data->fp) != mp->data.len) {
        mg_printf(nc, "%s",
                  "HTTP/1.1 500 Failed to write to a file\r\n"
                  "Content-Length: 0\r\n\r\n");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      data->bytes_written += mp->data.len;
      break;
    }
    case MG_EV_HTTP_PART_END: {
		long fileLen = ftell(data->fp);
      mg_printf(nc,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "%ld\n\n",
                fileLen);
      nc->flags |= MG_F_SEND_AND_CLOSE;

	  //提取临时文件名
//	  TCHAR strFileName[MAX_PATH]={0};
//	  GetFileNameFromFILEPtr(data->fp, strFileName);
//	  printf("[%s]\n", mp->file_name);

	  //读取、处理文件内容
	  fseek(data->fp, 0, SEEK_SET);
	  char* content = new char[fileLen+1];
	  if(content!=NULL)
	  {
		  memset(content, 0, fileLen+1);
		  fread(content,1,fileLen,data->fp);

		  //处理文件内容
		  process_xml(mp->file_name, content, fileLen+1);

		  delete [] content;
	  }

      fclose(data->fp);
      free(data);
      nc->user_data = NULL;
      break;
    }
  }
}


static void handle_upload_hop(struct mg_connection *nc, int ev, void *p) {
  struct file_writer_data *data = (struct file_writer_data *) nc->user_data;
  struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;


  switch (ev) {
    case MG_EV_HTTP_PART_BEGIN: {
      if (data == NULL) {
        data = (struct file_writer_data*)calloc(1, sizeof(struct file_writer_data));
        data->fp = tmpfile();
        data->bytes_written = 0;

        if (data->fp == NULL) {
          mg_printf(nc, "%s",
                    "HTTP/1.1 500 Failed to open a file\r\n"
                    "Content-Length: 0\r\n\r\n");
          nc->flags |= MG_F_SEND_AND_CLOSE;
          free(data);
          return;
        }
        nc->user_data = (void *) data;
      }
      break;
    }
    case MG_EV_HTTP_PART_DATA: {
      if (fwrite(mp->data.p, 1, mp->data.len, data->fp) != mp->data.len) {
        mg_printf(nc, "%s",
                  "HTTP/1.1 500 Failed to write to a file\r\n"
                  "Content-Length: 0\r\n\r\n");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      data->bytes_written += mp->data.len;
      break;
    }
    case MG_EV_HTTP_PART_END: {
		long fileLen = ftell(data->fp);
      mg_printf(nc,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "%ld\n\n",
                fileLen);
      nc->flags |= MG_F_SEND_AND_CLOSE;

	  //提取临时文件名
//	  TCHAR strFileName[MAX_PATH]={0};
//	  GetFileNameFromFILEPtr(data->fp, strFileName);
//	  printf("[%s]\n", strFileName);

	  //读取、处理文件内容
	  fseek(data->fp, 0, SEEK_SET);
	  char* content = new char[fileLen+1];
	  if(content!=NULL)
	  {
		  memset(content, 0, fileLen+1);
		  fread(content,1,fileLen,data->fp);

		  //处理文件内容
		  process_hop(mp->file_name, content, fileLen+1);

		  delete [] content;
	  }

      fclose(data->fp);
      free(data);
      nc->user_data = NULL;
      break;
    }
  }
}


static void handle_upload_cmd(struct mg_connection *nc, int ev, void *p) {
  struct file_writer_data *data = (struct file_writer_data *) nc->user_data;
  struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) p;

  switch (ev) {
    case MG_EV_HTTP_PART_BEGIN: {
      if (data == NULL) {
        data = (struct file_writer_data*)calloc(1, sizeof(struct file_writer_data));
        data->fp = tmpfile();
        data->bytes_written = 0;

        if (data->fp == NULL) {
          mg_printf(nc, "%s",
                    "HTTP/1.1 500 Failed to open a file\r\n"
                    "Content-Length: 0\r\n\r\n");
          nc->flags |= MG_F_SEND_AND_CLOSE;
          free(data);
          return;
        }
        nc->user_data = (void *) data;
      }
      break;
    }
    case MG_EV_HTTP_PART_DATA: {
      if (fwrite(mp->data.p, 1, mp->data.len, data->fp) != mp->data.len) {
        mg_printf(nc, "%s",
                  "HTTP/1.1 500 Failed to write to a file\r\n"
                  "Content-Length: 0\r\n\r\n");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      data->bytes_written += mp->data.len;
      break;
    }
    case MG_EV_HTTP_PART_END: {
		long fileLen = ftell(data->fp);
		bool ret = false;

	  //提取临时文件名
//	  TCHAR strFileName[MAX_PATH]={0};
//	  GetFileNameFromFILEPtr(data->fp, strFileName);
//	  printf("[%s]\n", strFileName);

	  //读取、处理文件内容
	  fseek(data->fp, 0, SEEK_SET);
	  char* content = new char[fileLen+1];
	  if(content!=NULL)
	  {
		  memset(content, 0, fileLen+1);
		  fread(content,1,fileLen,data->fp);

		  //处理文件内容
		  ret = process_cmd(mp->file_name, content, fileLen+1);

		  delete [] content;
	  }

      fclose(data->fp);
      free(data);
      nc->user_data = NULL;

	  //应答的 http 消息
	  if(ret)
	  {
		  mg_printf(nc,
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/plain\r\n"
					"Connection: close\r\n\r\n"
					"%ld\n\n",
					fileLen);
	  }
	  else
	  {
		  char info[128]={0};
		  sprintf(info, "HTTP/1.1 500 Failed: %s\r\nContent-Length: 0\r\n\r\n", mp->file_name);
        mg_printf(nc, "%s",info);
	  }
      nc->flags |= MG_F_SEND_AND_CLOSE;


      break;
    }
  }
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {

  if (ev == MG_EV_HTTP_REQUEST) {
    mg_serve_http(nc, (http_message *)ev_data, s_http_server_opts);
  }
}


int main(int argc, char* argv[])
{

//	s_http_port = sPort;

	struct mg_mgr mgr;
	struct mg_connection *c;

	mg_mgr_init(&mgr, NULL);
	c = mg_bind(&mgr, s_http_port, ev_handler);
	if (c == NULL) {
		fprintf(stderr, "!!! Error: Cannot start server on port %s\n", s_http_port);
		sleep(1);
		exit(EXIT_FAILURE);
	}

	s_http_server_opts.document_root = ".";  // Serve current directory
	mg_register_http_endpoint(c, "/upload_xml", handle_upload_xml MG_UD_ARG(NULL));
	mg_register_http_endpoint(c, "/upload_hop", handle_upload_hop MG_UD_ARG(NULL));
	mg_register_http_endpoint(c, "/upload_cmd", handle_upload_cmd MG_UD_ARG(NULL));


	// Set up HTTP server parameters
	mg_set_protocol_http_websocket(c);

	printf("Starting web server on port %s\n\n\n", s_http_port);
	for (;;) {
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);
	return 0;
}




int translateXml()
{
	//先读进来
	cout << "读取xml文件" << endl;
	TiXmlDocument mydoc("test.xml");//xml文档对象
	bool loadOk = mydoc.LoadFile();//加载文档
	if (!loadOk)
	{
		cout << "could not load the test file.Error:" << mydoc.ErrorDesc() << endl;
		exit(1);
	}

	//创建xml写工具，并进行初始化、。
	TiXmlDocument *writeDoc = new TiXmlDocument; //xml文档指针
	//文档格式声明
	TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "UTF-8", "yes");
	writeDoc->LinkEndChild(decl); //写入文档

	TiXmlElement* network = new TiXmlElement("network");//最高一层是network
	writeDoc->LinkEndChild(network);
	//locale="C" version="1.7" reference_time="15:23:03.000 Jun 28 2018" attribute_processing="explicit"相关属性
	network->SetAttribute("locale", "C");
	network->SetAttribute("version", "1.7");
	network->SetAttribute("attribute_processing", "explicit");



	//TDMAconfig节点。
	TiXmlElement *eplrs_config = new TiXmlElement("node");
	vector<string> lvId;//eplrs_config中属性参数与lvId有关。因为一个EPLRS的网络参数与lvId相对应。用于记录所有的旅的id。
	TiXmlElement *sitl_L=NULL;
	TiXmlElement *sitl_Y = NULL;
	TiXmlElement *sitl_L_inc = NULL;
	TiXmlElement *sitl_Y_inc = NULL;


	//依次遍历各个子网，边读边写
	TiXmlElement *RootElement = mydoc.RootElement();	//根元素, 对应于network
	for (TiXmlElement *data = RootElement->FirstChildElement(); data != NULL; data = data->NextSiblingElement()){
		cout << "data->Attribute(model)=" << data->Attribute("model") << endl;

		if (data->Attribute("model") != NULL&&strcmp(data->Attribute("model"), "JS_Command") == 0){//字符串比较，对JS_Command子网进行处理
			cout << "处理JS_Command" << endl;
			//进行处理
			TiXmlElement* JS_Command = new TiXmlElement("subnet");
			network->LinkEndChild(JS_Command);//从属于network
			//进行相关属性设置,<subnet name="JS_Command1" mobility="mobile">
			JS_Command->SetAttribute("name", data->Attribute("name"));
			JS_Command->SetAttribute("mobility", "mobile");
			//然后依次进行fbcb2转换、inc转换、mse_nc转换。然后进行链路mse_nc-inc转换、链路fbcb2-inc转换。
			//<node name="mse_nc" model="mse_nc" ignore_questions="true" min_match_score="strict matching">
			/*
			<attr name="network id" value="1"/>
			<attr name="father_id" value="1"/>
			<attr name="longitude" value="0"/>
			<attr name="latitude" value="0"/>
			<attr name="height" value="0"/>
			*/
			TiXmlElement* fbcb2 = new TiXmlElement("node");
			TiXmlElement* inc = new TiXmlElement("node");
			TiXmlElement* mse_ap = new TiXmlElement("node");
			TiXmlElement* mse_center = new TiXmlElement("node");
			TiXmlElement* mse_ap_inc = new TiXmlElement("link");
			TiXmlElement* mse_center_inc = new TiXmlElement("link");
			TiXmlElement* fbcb2_inc = new TiXmlElement("link");

			for (TiXmlElement *info = data->FirstChildElement(); info != NULL; info = info->NextSiblingElement()){
				//cout << "info->Attribute(model)=" << info->Attribute("model")<<endl;
				if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "fbcb2_model") == 0){//进行fbcb2处理
					JS_Command->LinkEndChild(fbcb2);
					fbcb2->SetAttribute("name", "fbcb2");
					fbcb2->SetAttribute("model", "fbcb2");
					fbcb2->SetAttribute("ignore_questions", "true");
					fbcb2->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="600"/>
					<attr name="user id" value="2"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "600");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					fbcb2->LinkEndChild(x_position);
					fbcb2->LinkEndChild(y_position);
					fbcb2->LinkEndChild(id);
					cout << "JS—fbcb2" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "inc_model") == 0){//进行inc处理
					JS_Command->LinkEndChild(inc);
					inc->SetAttribute("name", "inc");
					inc->SetAttribute("model", "INC");
					inc->SetAttribute("ignore_questions", "true");
					inc->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "300");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					inc->LinkEndChild(x_position);
					inc->LinkEndChild(y_position);
					inc->LinkEndChild(id);
					cout << "inc" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "mse_nc_model") == 0){//进行mse_nc处理

					JS_Command->LinkEndChild(mse_ap);
					JS_Command->LinkEndChild(mse_center);

					mse_ap->SetAttribute("name", "mse_ap");
					mse_ap->SetAttribute("model", "mse_ap");
					mse_ap->SetAttribute("ignore_questions", "true");
					mse_ap->SetAttribute("min_match_score", "strict matching");

					mse_center->SetAttribute("name", "mse_center");
					mse_center->SetAttribute("model", "mse_center");
					mse_center->SetAttribute("ignore_questions", "true");
					mse_center->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="0"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "-2");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));
					mse_ap->LinkEndChild(x_position);
					mse_ap->LinkEndChild(y_position);
					mse_ap->LinkEndChild(id);

					x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "2");
					y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");
					mse_center->LinkEndChild(x_position);
					mse_center->LinkEndChild(y_position);
					cout << "JS--mse_nc" << endl;
				}
				else if (strcmp(info->Attribute("name"), "network id") == 0){//进行网络信息设置处理及user id设置
					/*
					<!-- IP-->
					<attr name="IP Routing Parameters.count" value="1"/>
					<attr name="IP Routing Parameters [0].Interface Information.count" value="2"/>
					<attr name="IP Routing Parameters [0].Interface Information [1].Address" value="192.1.1.0"/>
					<attr name="IP Routing Parameters [0].Interface Information [1].Subnet Mask" value="Class C (natural)" symbolic="true"/>
				
					<attr name="Wireless LAN Parameters (IF1 P0).count" value="1"/>
					<attr name="Wireless LAN Parameters (IF1 P0) [0].BSS Identifier" value="1"/>
					*/
					TiXmlElement *ip1 = new TiXmlElement("attr");
					TiXmlElement *ip2 = new TiXmlElement("attr");
					TiXmlElement *ip3 = new TiXmlElement("attr");
					TiXmlElement *ip4 = new TiXmlElement("attr");
					TiXmlElement *w1 = new TiXmlElement("attr");
					TiXmlElement *w2 = new TiXmlElement("attr");
					string bssid = info->Attribute("value");
					string ipAddr = "192." + bssid + ".1.0";
					ip1->SetAttribute("name", "IP Routing Parameters.count");
					ip1->SetAttribute("value", "1");
					ip2->SetAttribute("name", "IP Routing Parameters [0].Interface Information.count");
					ip2->SetAttribute("value", "2");
					ip3->SetAttribute("name", "IP Routing Parameters [0].Interface Information [1].Address");
					ip3->SetAttribute("value", ipAddr.data());//这是ip地址
					ip4->SetAttribute("name", "IP Routing Parameters [0].Interface Information [1].Subnet Mask");
					ip4->SetAttribute("value", "Class C (natural)");
					ip4->SetAttribute("symbolic", "true");
					w1->SetAttribute("name", "Wireless LAN Parameters (IF1 P0).count");
					w1->SetAttribute("value", "1");
					w2->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].BSS Identifier");
					w2->SetAttribute("value", bssid.data());//这是BSSID
					cout << "mse_ap bssid=" << info->Attribute("value") << endl;
					mse_ap->LinkEndChild(ip1);
					mse_ap->LinkEndChild(ip2);
					mse_ap->LinkEndChild(ip3);
					mse_ap->LinkEndChild(ip4);
					mse_ap->LinkEndChild(w1);
					mse_ap->LinkEndChild(w2);

					/*
					<attr name="Mobile IPv4 Parameters.count" value="1"/>
					<attr name="Mobile IPv4 Parameters [0].Interface Information.count" value="1" symbolic="true"/>
					<attr name="Mobile IPv4 Parameters [0].Interface Information [0].Interface Name" value="IF1"/>
					<attr name="Mobile IPv4 Parameters [0].Interface Information [0].Agent Type" value="Home Agent/Foreign Agent" symbolic="true"/>
					<attr name="Mobile IPv4 Parameters [0].Interface Information [0].Mobile Router Configuration.count" value="1"/>
					<attr name="Mobile IPv4 Parameters [0].Interface Information [0].Mobile Router Configuration [0].Home Agent IP Address" value="192.1.1.0"/>
				
					*/
					TiXmlElement *mip1 = new TiXmlElement("attr");
					TiXmlElement *mip2 = new TiXmlElement("attr");
					TiXmlElement *mip3 = new TiXmlElement("attr");
					TiXmlElement *mip4 = new TiXmlElement("attr");
					TiXmlElement *mip5 = new TiXmlElement("attr");
					TiXmlElement *mip6 = new TiXmlElement("attr");
					mip1->SetAttribute("name", "Mobile IPv4 Parameters.count");
					mip1->SetAttribute("value", "1");
					mip2->SetAttribute("name", "Mobile IPv4 Parameters[0].Interface Information.count");
					mip2->SetAttribute("value", "1");
					mip2->SetAttribute("symbolic", "true");
					mip3->SetAttribute("name", "Mobile IPv4 Parameters [0].Interface Information [0].Interface Name");
					mip3->SetAttribute("value", "IF1");
					mip4->SetAttribute("name", "Mobile IPv4 Parameters [0].Interface Information [0].Agent Type");
					mip4->SetAttribute("value", "Home Agent/Foreign Agent");
					mip4->SetAttribute("symbolic", "true");
					mip5->SetAttribute("name", "Mobile IPv4 Parameters [0].Interface Information [0].Mobile Router Configuration.count");
					mip5->SetAttribute("value", "1");
					mip6->SetAttribute("name", "Mobile IPv4 Parameters [0].Interface Information [0].Mobile Router Configuration [0].Home Agent IP Address");
					mip6->SetAttribute("value", ipAddr.data());
					mse_ap->LinkEndChild(mip1);
					mse_ap->LinkEndChild(mip2);
					mse_ap->LinkEndChild(mip3);
					mse_ap->LinkEndChild(mip4);
					mse_ap->LinkEndChild(mip5);
					mse_ap->LinkEndChild(mip6);
					/*
					根据network id设置user id
					*/
					TiXmlElement *EidAttr = info->FirstChildElement();//
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", info->Attribute("value"));
					JS_Command->LinkEndChild(id);
				}
				else if (strcmp(info->Attribute("name"), "longitude") == 0){
					//经度处理
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", info->Attribute("value"));
					JS_Command->LinkEndChild(x_position);
				}
				else if (strcmp(info->Attribute("name"), "latitude") == 0){
					//纬度处理
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", info->Attribute("value"));
					JS_Command->LinkEndChild(y_position);
				}
				else if (strcmp(info->Attribute("name"), "height") == 0){
					//高度处理
					TiXmlElement *h_position = new TiXmlElement("attr");
					h_position->SetAttribute("name", "altitude");
					h_position->SetAttribute("value", info->Attribute("value"));
					JS_Command->LinkEndChild(h_position);
				}
				else
					cout << "其他JS_Command属性处理" << endl;
			}
			/*
			进行链路mse_nc-inc转换、链路fbcb2-inc转换。
			转换前格式：
			<link name="mse_nc_2-inc_4" model="100BaseT" class="" srcNode="mse_nc_2" destNode="inc_4"/>
			<link name = "fbcb2_3-inc_4" model = "100BaseT" class = "" srcNode = "fbcb2_3" destNode = "inc_4" / >
			转换后：
			<link name="mse_nc-inc" model="100BaseT" class="duplex" srcNode="mse_nc" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			*/

			JS_Command->LinkEndChild(mse_ap_inc);
			mse_ap_inc->SetAttribute("name", "mse_ap_inc");
			mse_ap_inc->SetAttribute("model", "100BaseT");
			mse_ap_inc->SetAttribute("class", "duplex");
			mse_ap_inc->SetAttribute("srcNode", "mse_ap");
			mse_ap_inc->SetAttribute("destNode", "inc");
			mse_ap_inc->SetAttribute("ignore_questions", "true");
			mse_ap_inc->SetAttribute("min_match_score", "strict matching");
			/*
			<attr name="transmitter a" value="mse_nc.eth_tx_0_0"/>
			<attr name="receiver a" value="mse_nc.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_0_0"/>
			<attr name="receiver b" value="inc.eth_rx_0_0"/>
			*/
			TiXmlElement *a1 = new TiXmlElement("attr");
			TiXmlElement *a2 = new TiXmlElement("attr");
			TiXmlElement *b1 = new TiXmlElement("attr");
			TiXmlElement *b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "mse_ap.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "mse_ap.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_0_0");
			mse_ap_inc->LinkEndChild(a1);
			mse_ap_inc->LinkEndChild(a2);
			mse_ap_inc->LinkEndChild(b1);
			mse_ap_inc->LinkEndChild(b2);

			JS_Command->LinkEndChild(fbcb2_inc);
			fbcb2_inc->SetAttribute("name", "fbcb2_inc");
			fbcb2_inc->SetAttribute("model", "100BaseT");
			fbcb2_inc->SetAttribute("class", "duplex");
			fbcb2_inc->SetAttribute("srcNode", "inc");
			fbcb2_inc->SetAttribute("destNode", "fbcb2");
			fbcb2_inc->SetAttribute("ignore_questions", "true");
			fbcb2_inc->SetAttribute("min_match_score", "strict matching");

			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");

			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_1_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_1_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "fbcb2.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "fbcb2.eth_rx_0_0");
			fbcb2_inc->LinkEndChild(a1);
			fbcb2_inc->LinkEndChild(a2);
			fbcb2_inc->LinkEndChild(b1);
			fbcb2_inc->LinkEndChild(b2);

			JS_Command->LinkEndChild(mse_center_inc);
			mse_center_inc->SetAttribute("name", "mse_center_inc");
			mse_center_inc->SetAttribute("model", "100BaseT");
			mse_center_inc->SetAttribute("class", "duplex");
			mse_center_inc->SetAttribute("srcNode", "inc");
			mse_center_inc->SetAttribute("destNode", "mse_center");
			mse_center_inc->SetAttribute("ignore_questions", "true");
			mse_center_inc->SetAttribute("min_match_score", "strict matching");

			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");

			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_2_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_2_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "mse_center.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "mse_center.eth_rx_0_0");
			mse_center_inc->LinkEndChild(a1);
			mse_center_inc->LinkEndChild(a2);
			mse_center_inc->LinkEndChild(b1);
			mse_center_inc->LinkEndChild(b2);


		}
		else if (strcmp(data->Attribute("model"), "L_Command") == 0){//字符串比较，对L_Command子网进行处理
			cout << "处理L_Command" << endl;
			//进行处理
			TiXmlElement* L_Command = new TiXmlElement("subnet");
			if (sitl_L == NULL)
				sitl_L = L_Command;
			network->LinkEndChild(L_Command);//从属于network
			//进行相关属性设置,<subnet name="L_Command1" mobility="mobile">
			L_Command->SetAttribute("name", data->Attribute("name"));
			L_Command->SetAttribute("mobility", "mobile");
			//然后依次进行fbcb2转换、mse_t转换、inc转换、tmg转换、ntdr_h转换、 eplrs_ncs转换、sincgars转换。
			//然后进行链路mse_nc-inc转换、链路fbcb2-inc转换。
			//<node name="mse_nc" model="mse_nc" ignore_questions="true" min_match_score="strict matching">
			/*
			<attr name="network id" value="1"/>
			<attr name="father_id" value="1"/>
			<attr name="longitude" value="0"/>
			<attr name="latitude" value="0"/>
			<attr name="height" value="0"/>
			*/
			TiXmlElement* fbcb2 = new TiXmlElement("node");
			TiXmlElement* mse_t = new TiXmlElement("node");
			TiXmlElement* inc = new TiXmlElement("node");
			if (sitl_L_inc == NULL)
				sitl_L_inc = inc;
			TiXmlElement* tmg = new TiXmlElement("node");
			TiXmlElement* ntdr_h = new TiXmlElement("node");
			TiXmlElement* eplrs_ncs = new TiXmlElement("node");
			TiXmlElement* sincgars = new TiXmlElement("node");

			//链路元素mse_t-tmg、tmg-inc、ntdr_h-inc、eplrs_ncs-inc、fbcb2-inc、sincgars-inc
			TiXmlElement* mse_t_tmg = new TiXmlElement("link");
			TiXmlElement* tmg_inc = new TiXmlElement("link");
			TiXmlElement* ntdr_h_inc = new TiXmlElement("link");
			TiXmlElement* eplrs_ncs_inc = new TiXmlElement("link");
			TiXmlElement* fbcb2_inc = new TiXmlElement("link");
			TiXmlElement* sincgars_inc = new TiXmlElement("link");

			for (TiXmlElement *info = data->FirstChildElement(); info != NULL; info = info->NextSiblingElement()){
				if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "fbcb2_model") == 0){//进行fbcb2处理
					L_Command->LinkEndChild(fbcb2);
					fbcb2->SetAttribute("name", "fbcb2");
					fbcb2->SetAttribute("model", "fbcb2");
					fbcb2->SetAttribute("ignore_questions", "true");
					fbcb2->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="600"/>
					<attr name="user id" value="2"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "1");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "5");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					fbcb2->LinkEndChild(x_position);
					fbcb2->LinkEndChild(y_position);
					fbcb2->LinkEndChild(id);
					cout << "L—fbcb2" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "mse_t_model") == 0){//进行mse_t处理
					L_Command->LinkEndChild(mse_t);
					mse_t->SetAttribute("name", "mse_t");
					mse_t->SetAttribute("model", "mse_t");
					mse_t->SetAttribute("ignore_questions", "true");
					mse_t->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="0"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "2.0");

					//设置业务参数
					/*
					Packet Format、Packet Size、Packet Interarrival Time、Stop Time、Start Time
					*/
					TiXmlElement *pf = new TiXmlElement("attr");
					pf->SetAttribute("name", "Packet Format");
					pf->SetAttribute("value", "data_s_pk");
					TiXmlElement *ps = new TiXmlElement("attr");
					ps->SetAttribute("name", "Packet Size");
					ps->SetAttribute("value", "constant(2048)");
					TiXmlElement *ptt = new TiXmlElement("attr");
					ptt->SetAttribute("name", "Packet Interarrival Time");
					ptt->SetAttribute("value", "poisson(10)");
					TiXmlElement *startT = new TiXmlElement("attr");
					startT->SetAttribute("name", "Start Time");
					startT->SetAttribute("value", "0");
					TiXmlElement *stopT = new TiXmlElement("attr");
					stopT->SetAttribute("name", "Stop Time");
					stopT->SetAttribute("value", "Infinity");


					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					mse_t->LinkEndChild(x_position);
					mse_t->LinkEndChild(y_position);
					mse_t->LinkEndChild(id);
					mse_t->LinkEndChild(pf);
					mse_t->LinkEndChild(ps);
					mse_t->LinkEndChild(ptt);
					//mse_t->LinkEndChild(startT);
					//mse_t->LinkEndChild(stopT);
					cout << "L--mse_t" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "inc_model") == 0){//进行inc处理
					L_Command->LinkEndChild(inc);
					inc->SetAttribute("name", "inc");
					inc->SetAttribute("model", "INC");
					inc->SetAttribute("ignore_questions", "true");
					inc->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					inc->LinkEndChild(x_position);
					inc->LinkEndChild(y_position);
					inc->LinkEndChild(id);
					cout << "L--inc" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "tmg_model") == 0){//进行tmg处理
					L_Command->LinkEndChild(tmg);
					tmg->SetAttribute("name", "tmg");
					tmg->SetAttribute("model", "TMG");
					tmg->SetAttribute("ignore_questions", "true");
					tmg->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "1.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					tmg->LinkEndChild(x_position);
					tmg->LinkEndChild(y_position);
					tmg->LinkEndChild(id);
					cout << "L--tmg" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "ntdr_h_model") == 0){//进行ntdr_h处理
					L_Command->LinkEndChild(ntdr_h);
					ntdr_h->SetAttribute("name", "ntdr_h");
					ntdr_h->SetAttribute("model", "ntdr_h");
					ntdr_h->SetAttribute("ignore_questions", "true");
					ntdr_h->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "1");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));
					/*
					<attr name="Wireless LAN Parameters (IF1 P0) [0].Channel Settings.count" value="1"/>
					<attr name="Wireless LAN Parameters (IF1 P0) [0].Channel Settings [0].Min Frequency" value="366"/>
					*/
					TiXmlElement *tmp = new TiXmlElement("attr");
					tmp->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].Channel Settings.count");
					tmp->SetAttribute("value", "1");
					TiXmlElement *pinlv = info->FirstChildElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement();//找到频率参数
					TiXmlElement *freq = new TiXmlElement("attr");
					freq->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].Channel Settings [0].Min Frequency");
					freq->SetAttribute("value", pinlv->Attribute("value"));

					ntdr_h->LinkEndChild(x_position);
					ntdr_h->LinkEndChild(y_position);
					ntdr_h->LinkEndChild(id);
					ntdr_h->LinkEndChild(tmp);
					ntdr_h->LinkEndChild(freq);
					cout << "L--ntdr_h" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "eplrs_ncs_model") == 0){//进行eplrs_ncs处理
					L_Command->LinkEndChild(eplrs_ncs);
					eplrs_ncs->SetAttribute("name", "eplrs_ncs");
					eplrs_ncs->SetAttribute("model", "eplrs_ncs");
					eplrs_ncs->SetAttribute("ignore_questions", "true");
					eplrs_ncs->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "-1");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					//设置业务参数
					/*
					Packet Format、Packet Size、Packet Interarrival Time、Stop Time、Start Time
					*/
					TiXmlElement *pf = new TiXmlElement("attr");
					pf->SetAttribute("name", "Packet Format");
					pf->SetAttribute("value", "data_s_pk");
					TiXmlElement *ps = new TiXmlElement("attr");
					ps->SetAttribute("name", "Packet Size");
					ps->SetAttribute("value", "constant(2048)");
					TiXmlElement *ptt = new TiXmlElement("attr");
					ptt->SetAttribute("name", "Packet Interarrival Time");
					ptt->SetAttribute("value", "poisson(10)");
					TiXmlElement *startT = new TiXmlElement("attr");
					startT->SetAttribute("name", "Start Time");
					startT->SetAttribute("value", "0");
					TiXmlElement *stopT = new TiXmlElement("attr");
					stopT->SetAttribute("name", "Stop Time");
					stopT->SetAttribute("value", "Infinity");
					eplrs_ncs->LinkEndChild(pf);
					eplrs_ncs->LinkEndChild(ps);
					eplrs_ncs->LinkEndChild(ptt);
					//eplrs_ncs->LinkEndChild(startT);
					//eplrs_ncs->LinkEndChild(stopT);

					eplrs_ncs->LinkEndChild(x_position);
					eplrs_ncs->LinkEndChild(y_position);
					eplrs_ncs->LinkEndChild(id);

					cout << "L--eplrs_ncs" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "sincgars_model") == 0){//进行sincgars处理
					L_Command->LinkEndChild(sincgars);
					sincgars->SetAttribute("name", "sincgars");
					sincgars->SetAttribute("model", "sincgars");
					sincgars->SetAttribute("ignore_questions", "true");
					sincgars->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "-1");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					sincgars->LinkEndChild(x_position);
					sincgars->LinkEndChild(y_position);
					sincgars->LinkEndChild(id);
					cout << "L--sincgars" << endl;
				}
				else if (strcmp(info->Attribute("name"), "network id") == 0){//user id设置及ntdr_h进行网络信息设置处理，eplrs_ncs 配置
					/*
					<!-- IP-->
					<attr name="Wireless LAN Parameters (IF1 P0).count" value="1"/>
					<attr name="Wireless LAN Parameters (IF1 P0) [0].BSS Identifier" value="51"/>
					*/
					TiXmlElement *w1 = new TiXmlElement("attr");
					TiXmlElement *w2 = new TiXmlElement("attr");

					w1->SetAttribute("name", "Wireless LAN Parameters (IF1 P0).count");
					w1->SetAttribute("value", "1");
					w2->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].BSS Identifier");
					w2->SetAttribute("value", info->Attribute("value"));//这是BSSID
					ntdr_h->LinkEndChild(w1);
					ntdr_h->LinkEndChild(w2);

					/*
					根据network id设置user id
					*/
					TiXmlElement *EidAttr = info->FirstChildElement();//
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", info->Attribute("value"));
					L_Command->LinkEndChild(id);

					/*
					eplrs 配置
					<attr name="Network Name (IF1 P0)" value="1"/>
					*/
					TiXmlElement *netid = new TiXmlElement("attr");
					netid->SetAttribute("name", "Network Name (IF1 P0)");
					netid->SetAttribute("value", info->Attribute("value"));
					string num = info->Attribute("value");
					lvId.push_back(num);
					eplrs_ncs->LinkEndChild(netid);

				}
				else if (strcmp(info->Attribute("name"), "father_id") == 0){//mse_t进行网络信息设置处理，mobile ip配置
					/*
					<attr name="Wireless LAN Parameters (IF1 P0).count" value="1"/>
					<attr name="Wireless LAN Parameters (IF1 P0) [0].BSS Identifier" value="1"/>
					*/
					TiXmlElement *w1 = new TiXmlElement("attr");
					TiXmlElement *w2 = new TiXmlElement("attr");

					w1->SetAttribute("name", "Wireless LAN Parameters (IF1 P0).count");
					w1->SetAttribute("value", "1");
					w2->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].BSS Identifier");
					w2->SetAttribute("value", info->Attribute("value"));//这是从属于JS的BSSID
					mse_t->LinkEndChild(w1);
					mse_t->LinkEndChild(w2);
					/*
						<attr name="Mobile IP Host Parameters.count" value="1"/>
						<attr name="Mobile IP Host Parameters [0].Home Agent IP Address" value="192.1.1.0"/>
					*/
					string bssid = info->Attribute("value");
					string ipAddr = "192." + bssid + ".1.0";
					TiXmlElement *m1 = new TiXmlElement("attr");
					TiXmlElement *m2 = new TiXmlElement("attr");
					m1->SetAttribute("name", "Mobile IP Host Parameters.count");
					m1->SetAttribute("value", "1");
					m2->SetAttribute("name", "Mobile IP Host Parameters [0].Home Agent IP Address");
					m2->SetAttribute("value", ipAddr.data());//这是从属于JS的BSSID
					mse_t->LinkEndChild(m1);
					mse_t->LinkEndChild(m2);

				}
				else if (strcmp(info->Attribute("name"), "longitude") == 0){
					//经度处理
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", info->Attribute("value"));
					L_Command->LinkEndChild(x_position);
				}
				else if (strcmp(info->Attribute("name"), "latitude") == 0){
					//纬度处理
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", info->Attribute("value"));
					L_Command->LinkEndChild(y_position);
				}
				else if (strcmp(info->Attribute("name"), "height") == 0){
					//高度处理
					TiXmlElement *h_position = new TiXmlElement("attr");
					h_position->SetAttribute("name", "altitude");
					h_position->SetAttribute("value", info->Attribute("value"));
					L_Command->LinkEndChild(h_position);
				}
				else
					cout << "其他L_Command属性处理" << endl;


			}
			/*
			依次进行链路转换
			mse_t-tmg、tmg-inc、ntdr_h-inc、eplrs_ncs-inc、fbcb2-inc、sincgars-inc

			*/
			/*	mse_t-tmg
			<attr name="transmitter a" value="mse_t.eth_tx_0_0"/>
			<attr name="receiver a" value="mse_t.eth_rx_0_0"/>
			<attr name="transmitter b" value="tmg.eth_tx_0_0"/>
			<attr name="receiver b" value="tmg.eth_rx_0_0"/>
			<link name="mse_t-tmg" model="100BaseT" class="duplex" srcNode="mse_t" destNode="tmg" ignore_questions="true" min_match_score="strict matching">
			*/
			L_Command->LinkEndChild(mse_t_tmg);
			mse_t_tmg->SetAttribute("name", "mse_t_tmg");
			mse_t_tmg->SetAttribute("model", "100BaseT");
			mse_t_tmg->SetAttribute("class", "duplex");
			mse_t_tmg->SetAttribute("srcNode", "mse_t");
			mse_t_tmg->SetAttribute("destNode", "tmg");
			mse_t_tmg->SetAttribute("ignore_questions", "true");
			mse_t_tmg->SetAttribute("min_match_score", "strict matching");
			TiXmlElement *a1 = new TiXmlElement("attr");
			TiXmlElement *a2 = new TiXmlElement("attr");
			TiXmlElement *b1 = new TiXmlElement("attr");
			TiXmlElement *b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "mse_t.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "mse_t.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "tmg.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "tmg.eth_rx_0_0");
			mse_t_tmg->LinkEndChild(a1);
			mse_t_tmg->LinkEndChild(a2);
			mse_t_tmg->LinkEndChild(b1);
			mse_t_tmg->LinkEndChild(b2);

			/*	tmg-inc
			<attr name="transmitter a" value="tmg.eth_tx_1_0"/>
			<attr name="receiver a" value="tmg.eth_rx_1_0"/>
			<attr name="transmitter b" value="inc.eth_tx_0_0"/>
			<attr name="receiver b" value="inc.eth_rx_0_0"/>
			<link name="tmg-inc" model="100BaseT" class="duplex" srcNode="tmg" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			*/
			L_Command->LinkEndChild(tmg_inc);
			tmg_inc->SetAttribute("name", "tmg_inc");
			tmg_inc->SetAttribute("model", "100BaseT");
			tmg_inc->SetAttribute("class", "duplex");
			tmg_inc->SetAttribute("srcNode", "tmg");
			tmg_inc->SetAttribute("destNode", "inc");
			tmg_inc->SetAttribute("ignore_questions", "true");
			tmg_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "tmg.eth_tx_1_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "tmg.eth_rx_1_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_0_0");
			tmg_inc->LinkEndChild(a1);
			tmg_inc->LinkEndChild(a2);
			tmg_inc->LinkEndChild(b1);
			tmg_inc->LinkEndChild(b2);

			/*	ntdr_h-inc
			<attr name="transmitter a" value="inc.eth_tx_1_0"/>
			<attr name="receiver a" value="inc.eth_rx_1_0"/>
			<attr name="transmitter b" value="ntdr_h.eth_tx_0_0"/>
			<attr name="receiver b" value="ntdr_h.eth_rx_0_0"/>
			<link name="ntdr_h-inc" model="100BaseT" class="duplex" srcNode="inc" destNode="ntdr_h" ignore_questions="true" min_match_score="strict matching">
			*/
			L_Command->LinkEndChild(ntdr_h_inc);
			ntdr_h_inc->SetAttribute("name", "ntdr_h_inc");
			ntdr_h_inc->SetAttribute("model", "100BaseT");
			ntdr_h_inc->SetAttribute("class", "duplex");
			ntdr_h_inc->SetAttribute("srcNode", "inc");
			ntdr_h_inc->SetAttribute("destNode", "ntdr_h");
			ntdr_h_inc->SetAttribute("ignore_questions", "true");
			ntdr_h_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_1_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_1_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "ntdr_h.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "ntdr_h.eth_rx_0_0");
			ntdr_h_inc->LinkEndChild(a1);
			ntdr_h_inc->LinkEndChild(a2);
			ntdr_h_inc->LinkEndChild(b1);
			ntdr_h_inc->LinkEndChild(b2);

			/*
			eplrs_ncs-inc
			<attr name="transmitter a" value="inc.eth_tx_2_0"/>
			<attr name="receiver a" value="inc.eth_rx_2_0"/>
			<attr name="transmitter b" value="eplrs_ncs.eth_tx_0_0"/>
			<attr name="receiver b" value="eplrs_ncs.eth_rx_0_0"/>
			<link name="eplrs_ncs-inc" model="100BaseT" class="duplex" srcNode="inc" destNode="eplrs_ncs" ignore_questions="true" min_match_score="strict matching">
			*/
			L_Command->LinkEndChild(eplrs_ncs_inc);
			eplrs_ncs_inc->SetAttribute("name", "eplrs_ncs_inc");
			eplrs_ncs_inc->SetAttribute("model", "100BaseT");
			eplrs_ncs_inc->SetAttribute("class", "duplex");
			eplrs_ncs_inc->SetAttribute("srcNode", "inc");
			eplrs_ncs_inc->SetAttribute("destNode", "eplrs_ncs");
			eplrs_ncs_inc->SetAttribute("ignore_questions", "true");
			eplrs_ncs_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_2_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_2_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "eplrs_ncs.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "eplrs_ncs.eth_rx_0_0");
			eplrs_ncs_inc->LinkEndChild(a1);
			eplrs_ncs_inc->LinkEndChild(a2);
			eplrs_ncs_inc->LinkEndChild(b1);
			eplrs_ncs_inc->LinkEndChild(b2);

			/*
			fbcb2-inc
			<attr name="transmitter a" value="fbcb2.eth_tx_0_0"/>
			<attr name="receiver a" value="fbcb2.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_3_0"/>
			<attr name="receiver b" value="inc.eth_rx_3_0"/>
			<link name="fbcb2-inc" model="100BaseT" class="duplex" srcNode="fbcb2" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			*/
			L_Command->LinkEndChild(fbcb2_inc);
			fbcb2_inc->SetAttribute("name", "fbcb2_inc");
			fbcb2_inc->SetAttribute("model", "100BaseT");
			fbcb2_inc->SetAttribute("class", "duplex");
			fbcb2_inc->SetAttribute("srcNode", "fbcb2");
			fbcb2_inc->SetAttribute("destNode", "inc");
			fbcb2_inc->SetAttribute("ignore_questions", "true");
			fbcb2_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "fbcb2.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "fbcb2.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_3_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_3_0");
			fbcb2_inc->LinkEndChild(a1);
			fbcb2_inc->LinkEndChild(a2);
			fbcb2_inc->LinkEndChild(b1);
			fbcb2_inc->LinkEndChild(b2);
			/*
			sincgars-inc
			<attr name="transmitter a" value="sincgars.eth_tx_0_0"/>
			<attr name="receiver a" value="sincgars.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_4_0"/>
			<attr name="receiver b" value="inc.eth_rx_4_0"/>
			<link name="sincgars-inc" model="100BaseT" class="duplex" srcNode="sincgars" destNode="inc" ignore_questions="true" min_match_score="strict matching">

			*/
			L_Command->LinkEndChild(sincgars_inc);
			sincgars_inc->SetAttribute("name", "sincgars_inc");
			sincgars_inc->SetAttribute("model", "100BaseT");
			sincgars_inc->SetAttribute("class", "duplex");
			sincgars_inc->SetAttribute("srcNode", "sincgars");
			sincgars_inc->SetAttribute("destNode", "inc");
			sincgars_inc->SetAttribute("ignore_questions", "true");
			sincgars_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "sincgars.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "sincgars.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_4_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_4_0");
			sincgars_inc->LinkEndChild(a1);
			sincgars_inc->LinkEndChild(a2);
			sincgars_inc->LinkEndChild(b1);
			sincgars_inc->LinkEndChild(b2);

		}
		else if (strcmp(data->Attribute("model"), "Y_Command") == 0){//字符串比较，对Y_Command子网进行处理
			cout << "处理Y_Command" << endl;
			TiXmlElement* Y_Command = new TiXmlElement("subnet");
			if (sitl_Y==NULL)
				sitl_Y = Y_Command;
			network->LinkEndChild(Y_Command);//从属于network
			//进行相关属性设置,<subnet name="Y_Command1" mobility="mobile">
			Y_Command->SetAttribute("name", data->Attribute("name"));
			Y_Command->SetAttribute("mobility", "mobile");
			//然后依次进行sincgars转换、inc转换、eplrs_rs转换、fbcb2、ntdr_s。
			//然后进行链路sincgars_inc、fbcb2_inc、eplrs_rs_inc、ntdr_s_inc转换。
			/*
			<attr name="network id" value="1"/>
			<attr name="father_id" value="1"/>
			<attr name="longitude" value="0"/>
			<attr name="latitude" value="0"/>
			<attr name="height" value="0"/>
			*/
			TiXmlElement* sincgars = new TiXmlElement("node");
			TiXmlElement* inc = new TiXmlElement("node");
			if (sitl_Y_inc==NULL)
				sitl_Y_inc = inc;
			TiXmlElement* eplrs_rs = new TiXmlElement("node");
			TiXmlElement* fbcb2 = new TiXmlElement("node");
			TiXmlElement* ntdr_s = new TiXmlElement("node");

			TiXmlElement* sincgars_inc = new TiXmlElement("link");
			TiXmlElement* fbcb2_inc = new TiXmlElement("link");
			TiXmlElement* eplrs_rs_inc = new TiXmlElement("link");
			TiXmlElement* ntdr_s_inc = new TiXmlElement("link");

			for (TiXmlElement *info = data->FirstChildElement(); info != NULL; info = info->NextSiblingElement()){
				if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "sincgars_model") == 0){//进行sincgars处理
					Y_Command->LinkEndChild(sincgars);
					sincgars->SetAttribute("name", "sincgars");
					sincgars->SetAttribute("model", "sincgars");
					sincgars->SetAttribute("ignore_questions", "true");
					sincgars->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "-1");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					sincgars->LinkEndChild(x_position);
					sincgars->LinkEndChild(y_position);
					sincgars->LinkEndChild(id);
					cout << "Y--sincgars" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "inc_model") == 0){//进行inc处理
					Y_Command->LinkEndChild(inc);
					inc->SetAttribute("name", "inc");
					inc->SetAttribute("model", "INC");
					inc->SetAttribute("ignore_questions", "true");
					inc->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					inc->LinkEndChild(x_position);
					inc->LinkEndChild(y_position);
					inc->LinkEndChild(id);
					cout << "Y--inc" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "eplrs_rs_model") == 0){//进行eplrs_rs处理
					Y_Command->LinkEndChild(eplrs_rs);
					eplrs_rs->SetAttribute("name", "eplrs_rs");
					eplrs_rs->SetAttribute("model", "eplrs_rs");
					eplrs_rs->SetAttribute("ignore_questions", "true");
					eplrs_rs->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "1.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					//设置业务参数
					/*
					Packet Format、Packet Size、Packet Interarrival Time、Stop Time、Start Time
					*/
					TiXmlElement *pf = new TiXmlElement("attr");
					pf->SetAttribute("name", "Packet Format");
					pf->SetAttribute("value", "data_s_pk");
					TiXmlElement *ps = new TiXmlElement("attr");
					ps->SetAttribute("name", "Packet Size");
					ps->SetAttribute("value", "constant(2048)");
					TiXmlElement *ptt = new TiXmlElement("attr");
					ptt->SetAttribute("name", "Packet Interarrival Time");
					ptt->SetAttribute("value", "poisson(10)");
					TiXmlElement *startT = new TiXmlElement("attr");
					startT->SetAttribute("name", "Start Time");
					startT->SetAttribute("value", "0");
					TiXmlElement *stopT = new TiXmlElement("attr");
					stopT->SetAttribute("name", "Stop Time");
					stopT->SetAttribute("value", "Infinity");
					eplrs_rs->LinkEndChild(pf);
					eplrs_rs->LinkEndChild(ps);
					eplrs_rs->LinkEndChild(ptt);
					//eplrs_rs->LinkEndChild(startT);
					//eplrs_rs->LinkEndChild(stopT);

					eplrs_rs->LinkEndChild(x_position);
					eplrs_rs->LinkEndChild(y_position);
					eplrs_rs->LinkEndChild(id);
					cout << "Y--eplrs_rs" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "fbcb2_model") == 0){//进行fbcb2处理
					Y_Command->LinkEndChild(fbcb2);
					fbcb2->SetAttribute("name", "fbcb2");
					fbcb2->SetAttribute("model", "fbcb2");
					fbcb2->SetAttribute("ignore_questions", "true");
					fbcb2->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="600"/>
					<attr name="user id" value="2"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "1");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "5");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					fbcb2->LinkEndChild(x_position);
					fbcb2->LinkEndChild(y_position);
					fbcb2->LinkEndChild(id);
					cout << "Y—fbcb2" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "ntdr_s_model") == 0){//进行ntdr_s处理
					Y_Command->LinkEndChild(ntdr_s);
					ntdr_s->SetAttribute("name", "ntdr_s");
					ntdr_s->SetAttribute("model", "ntdr_s");
					ntdr_s->SetAttribute("ignore_questions", "true");
					ntdr_s->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "1.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					/*
					<attr name="Wireless LAN Parameters (IF1 P0) [0].Channel Settings.count" value="1"/>
					<attr name="Wireless LAN Parameters (IF1 P0) [0].Channel Settings [0].Min Frequency" value="366"/>
					*/
					TiXmlElement *tmp = new TiXmlElement("attr");
					tmp->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].Channel Settings.count");
					tmp->SetAttribute("value", "1");
					TiXmlElement *pinlv = info->FirstChildElement()->NextSiblingElement()->NextSiblingElement()->NextSiblingElement();//找到频率参数
					TiXmlElement *freq = new TiXmlElement("attr");
					freq->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].Channel Settings [0].Min Frequency");
					freq->SetAttribute("value", pinlv->Attribute("value"));

					//设置业务参数
					/*
					Packet Format、Packet Size、Packet Interarrival Time、Stop Time、Start Time
					*/
					TiXmlElement *pf = new TiXmlElement("attr");
					pf->SetAttribute("name", "Packet Format");
					pf->SetAttribute("value", "data_s_pk");
					TiXmlElement *ps = new TiXmlElement("attr");
					ps->SetAttribute("name", "Packet Size");
					ps->SetAttribute("value", "constant(2048)");
					TiXmlElement *ptt = new TiXmlElement("attr");
					ptt->SetAttribute("name", "Packet Interarrival Time");
					ptt->SetAttribute("value", "poisson(10)");
					TiXmlElement *startT = new TiXmlElement("attr");
					startT->SetAttribute("name", "Start Time");
					startT->SetAttribute("value", "0");
					TiXmlElement *stopT = new TiXmlElement("attr");
					stopT->SetAttribute("name", "Stop Time");
					stopT->SetAttribute("value", "Infinity");
					ntdr_s->LinkEndChild(pf);
					ntdr_s->LinkEndChild(ps);
					ntdr_s->LinkEndChild(ptt);
					//ntdr_s->LinkEndChild(startT);
				//	ntdr_s->LinkEndChild(stopT);

					ntdr_s->LinkEndChild(x_position);
					ntdr_s->LinkEndChild(y_position);
					ntdr_s->LinkEndChild(id);
					ntdr_s->LinkEndChild(tmp);
					ntdr_s->LinkEndChild(freq);
					cout << "Y--ntdr_s" << endl;
				}
				else if (strcmp(info->Attribute("name"), "network id") == 0){//user id设置
					/*
					根据network id设置user id
					*/
					TiXmlElement *EidAttr = info->FirstChildElement();//
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", info->Attribute("value"));
					Y_Command->LinkEndChild(id);
				}
				else if (strcmp(info->Attribute("name"), "father_id") == 0){//ntdr_s进行网络信息设置处理及eplrs_rs设置
					/*
					<!-- BSSID-->
					<attr name="Wireless LAN Parameters (IF1 P0).count" value="1"/>
					<attr name="Wireless LAN Parameters (IF1 P0) [0].BSS Identifier" value="51"/>
					*/
					TiXmlElement *w1 = new TiXmlElement("attr");
					TiXmlElement *w2 = new TiXmlElement("attr");

					w1->SetAttribute("name", "Wireless LAN Parameters (IF1 P0).count");
					w1->SetAttribute("value", "1");
					w2->SetAttribute("name", "Wireless LAN Parameters (IF1 P0) [0].BSS Identifier");
					w2->SetAttribute("value", info->Attribute("value"));//这是BSSID
					ntdr_s->LinkEndChild(w1);
					ntdr_s->LinkEndChild(w2);

					/*
					eplrs 配置
					<attr name="Network Name (IF1 P0)" value="1"/>
					*/
					TiXmlElement *netid = new TiXmlElement("attr");
					netid->SetAttribute("name", "Network Name (IF1 P0)");
					netid->SetAttribute("value", info->Attribute("value"));
					string num = info->Attribute("value");
					eplrs_rs->LinkEndChild(netid);

				}
				else if (strcmp(info->Attribute("name"), "longitude") == 0){
					//经度处理
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", info->Attribute("value"));
					Y_Command->LinkEndChild(x_position);
				}
				else if (strcmp(info->Attribute("name"), "latitude") == 0){
					//纬度处理
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", info->Attribute("value"));
					Y_Command->LinkEndChild(y_position);
				}
				else if (strcmp(info->Attribute("name"), "height") == 0){
					//高度处理
					TiXmlElement *h_position = new TiXmlElement("attr");
					h_position->SetAttribute("name", "altitude");
					h_position->SetAttribute("value", info->Attribute("value"));
					Y_Command->LinkEndChild(h_position);
				}
				else
					cout << "其他Y_Command属性处理" << endl;
			}
			/*
			链路设置 sincgars_inc、fbcb2_inc、eplrs_rs_inc、ntdr_s_inc
			*/

			/*
			sincgars-inc
			<attr name="transmitter a" value="sincgars.eth_tx_0_0"/>
			<attr name="receiver a" value="sincgars.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_0_0"/>
			<attr name="receiver b" value="inc.eth_rx_0_0"/>
			<link name="sincgars-inc" model="100BaseT" class="duplex" srcNode="sincgars" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			*/
			Y_Command->LinkEndChild(sincgars_inc);
			sincgars_inc->SetAttribute("name", "sincgars_inc");
			sincgars_inc->SetAttribute("model", "100BaseT");
			sincgars_inc->SetAttribute("class", "duplex");
			sincgars_inc->SetAttribute("srcNode", "sincgars");
			sincgars_inc->SetAttribute("destNode", "inc");
			sincgars_inc->SetAttribute("ignore_questions", "true");
			sincgars_inc->SetAttribute("min_match_score", "strict matching");
			TiXmlElement *a1 = new TiXmlElement("attr");
			TiXmlElement *a2 = new TiXmlElement("attr");
			TiXmlElement *b1 = new TiXmlElement("attr");
			TiXmlElement *b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "sincgars.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "sincgars.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_0_0");
			sincgars_inc->LinkEndChild(a1);
			sincgars_inc->LinkEndChild(a2);
			sincgars_inc->LinkEndChild(b1);
			sincgars_inc->LinkEndChild(b2);

			/*
			fbcb2_inc
			<link name="fbcb2-inc" model="100BaseT" class="duplex" srcNode="inc" destNode="fbcb2" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="inc.eth_tx_1_0"/>
			<attr name="receiver a" value="inc.eth_rx_1_0"/>
			<attr name="transmitter b" value="fbcb2.eth_tx_0_0"/>
			<attr name="receiver b" value="fbcb2.eth_rx_0_0"/>

			*/
			Y_Command->LinkEndChild(fbcb2_inc);
			fbcb2_inc->SetAttribute("name", "fbcb2_inc");
			fbcb2_inc->SetAttribute("model", "100BaseT");
			fbcb2_inc->SetAttribute("class", "duplex");
			fbcb2_inc->SetAttribute("srcNode", "inc");
			fbcb2_inc->SetAttribute("destNode", "fbcb2");
			fbcb2_inc->SetAttribute("ignore_questions", "true");
			fbcb2_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_1_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_1_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "fbcb2.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "fbcb2.eth_rx_0_0");
			fbcb2_inc->LinkEndChild(a1);
			fbcb2_inc->LinkEndChild(a2);
			fbcb2_inc->LinkEndChild(b1);
			fbcb2_inc->LinkEndChild(b2);

			/*
			eplrs_rs_inc
			<link name="eplrs_rs-inc" model="100BaseT" class="duplex" srcNode="eplrs_rs" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="eplrs_rs.eth_tx_0_0"/>
			<attr name="receiver a" value="eplrs_rs.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_2_0"/>
			<attr name="receiver b" value="inc.eth_rx_2_0"/>
			*/
			Y_Command->LinkEndChild(eplrs_rs_inc);
			eplrs_rs_inc->SetAttribute("name", "eplrs_rs_inc");
			eplrs_rs_inc->SetAttribute("model", "100BaseT");
			eplrs_rs_inc->SetAttribute("class", "duplex");
			eplrs_rs_inc->SetAttribute("srcNode", "eplrs_rs");
			eplrs_rs_inc->SetAttribute("destNode", "inc");
			eplrs_rs_inc->SetAttribute("ignore_questions", "true");
			eplrs_rs_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "eplrs_rs.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "eplrs_rs.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_2_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_2_0");
			eplrs_rs_inc->LinkEndChild(a1);
			eplrs_rs_inc->LinkEndChild(a2);
			eplrs_rs_inc->LinkEndChild(b1);
			eplrs_rs_inc->LinkEndChild(b2);

			/*
			ntdr_s_inc
			<link name="ntdr_s-inc" model="100BaseT" class="duplex" srcNode="inc" destNode="ntdr_s" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="inc.eth_tx_3_0"/>
			<attr name="receiver a" value="inc.eth_rx_3_0"/>
			<attr name="transmitter b" value="ntdr_s.eth_tx_0_0"/>
			<attr name="receiver b" value="ntdr_s.eth_rx_0_0"/>
			*/
			Y_Command->LinkEndChild(ntdr_s_inc);
			ntdr_s_inc->SetAttribute("name", "ntdr_s_inc");
			ntdr_s_inc->SetAttribute("model", "100BaseT");
			ntdr_s_inc->SetAttribute("class", "duplex");
			ntdr_s_inc->SetAttribute("srcNode", "inc");
			ntdr_s_inc->SetAttribute("destNode", "ntdr_s");
			ntdr_s_inc->SetAttribute("ignore_questions", "true");
			ntdr_s_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_3_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_3_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "ntdr_s.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "ntdr_s.eth_rx_0_0");
			ntdr_s_inc->LinkEndChild(a1);
			ntdr_s_inc->LinkEndChild(a2);
			ntdr_s_inc->LinkEndChild(b1);
			ntdr_s_inc->LinkEndChild(b2);

		}
		else if (strcmp(data->Attribute("model"), "Soldier") == 0){//字符串比较，对Soldier子网进行处理
			cout << "处理Soldier" << endl;
			TiXmlElement* Soldier = new TiXmlElement("subnet");
			network->LinkEndChild(Soldier);//从属于network
			//进行相关属性设置,<subnet name="Soldier" mobility="mobile">
			Soldier->SetAttribute("name", data->Attribute("name"));
			Soldier->SetAttribute("mobility", "mobile");
			//然后依次进行sincgars转换、inc转换。然后进行链路sincgars-inc转换。
			//<node name="mse_nc" model="mse_nc" ignore_questions="true" min_match_score="strict matching">
			/*
			<attr name="network id" value="1"/>
			<attr name="father_id" value="1"/>
			<attr name="longitude" value="0"/>
			<attr name="latitude" value="0"/>
			<attr name="height" value="0"/>
			*/
			TiXmlElement* sincgars = new TiXmlElement("node");
			TiXmlElement* inc = new TiXmlElement("node");
			TiXmlElement* sincgars_inc = new TiXmlElement("link");

			for (TiXmlElement *info = data->FirstChildElement(); info != NULL; info = info->NextSiblingElement()){
				if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "sincgars_model") == 0){//进行sincgars处理
					Soldier->LinkEndChild(sincgars);
					sincgars->SetAttribute("name", "sincgars");
					sincgars->SetAttribute("model", "sincgars");
					sincgars->SetAttribute("ignore_questions", "true");
					sincgars->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					sincgars->LinkEndChild(x_position);
					sincgars->LinkEndChild(y_position);
					sincgars->LinkEndChild(id);
					cout << "Soldier--sincgars" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "inc_model") == 0){//进行inc处理
					Soldier->LinkEndChild(inc);
					inc->SetAttribute("name", "inc");
					inc->SetAttribute("model", "INC");
					inc->SetAttribute("ignore_questions", "true");
					inc->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "-2");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					inc->LinkEndChild(x_position);
					inc->LinkEndChild(y_position);
					inc->LinkEndChild(id);
					cout << "Soldier--inc" << endl;
				}
				else if (strcmp(info->Attribute("name"), "network id") == 0){//user id设置
					/*
					根据network id设置user id
					*/
					TiXmlElement *EidAttr = info->FirstChildElement();//
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", info->Attribute("value"));
					Soldier->LinkEndChild(id);
				}
				else if (strcmp(info->Attribute("name"), "longitude") == 0){
					//经度处理
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", info->Attribute("value"));
					Soldier->LinkEndChild(x_position);
				}
				else if (strcmp(info->Attribute("name"), "latitude") == 0){
					//纬度处理
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", info->Attribute("value"));
					Soldier->LinkEndChild(y_position);
				}
				else if (strcmp(info->Attribute("name"), "height") == 0){
					//高度处理
					TiXmlElement *h_position = new TiXmlElement("attr");
					h_position->SetAttribute("name", "altitude");
					h_position->SetAttribute("value", info->Attribute("value"));
					Soldier->LinkEndChild(h_position);
				}
				else
					cout << "其他Soldier属性处理" << endl;
			}

			Soldier->LinkEndChild(sincgars_inc);
			sincgars_inc->SetAttribute("name", "sincgars_inc");
			sincgars_inc->SetAttribute("model", "100BaseT");
			sincgars_inc->SetAttribute("class", "duplex");
			sincgars_inc->SetAttribute("srcNode", "sincgars");
			sincgars_inc->SetAttribute("destNode", "inc");
			sincgars_inc->SetAttribute("ignore_questions", "true");
			sincgars_inc->SetAttribute("min_match_score", "strict matching");
			TiXmlElement *a1 = new TiXmlElement("attr");
			TiXmlElement *a2 = new TiXmlElement("attr");
			TiXmlElement *b1 = new TiXmlElement("attr");
			TiXmlElement *b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "sincgars.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "sincgars.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_0_0");
			sincgars_inc->LinkEndChild(a1);
			sincgars_inc->LinkEndChild(a2);
			sincgars_inc->LinkEndChild(b1);
			sincgars_inc->LinkEndChild(b2);

		}
		else if (strcmp(data->Attribute("model"), "Plane") == 0){//字符串比较，对Plane子网进行处理
			cout << "处理Plane" << endl;
			TiXmlElement* Plane = new TiXmlElement("subnet");
			network->LinkEndChild(Plane);//从属于network
			//进行相关属性设置,<subnet name="JS_Command1" mobility="mobile">
			Plane->SetAttribute("name", data->Attribute("name"));
			Plane->SetAttribute("mobility", "mobile");
			//然后依次进行eplrs_rs转换、inc转换、fbcb2转换。然后进行链路eplrs_rs_inc转换、链路fbcb2-inc转换。
			//<node name="mse_nc" model="mse_nc" ignore_questions="true" min_match_score="strict matching">
			/*
			<attr name="network id" value="1"/>
			<attr name="father_id" value="1"/>
			<attr name="longitude" value="0"/>
			<attr name="latitude" value="0"/>
			<attr name="height" value="0"/>
			*/
			TiXmlElement* fbcb2 = new TiXmlElement("node");
			TiXmlElement* inc = new TiXmlElement("node");
			TiXmlElement* eplrs_rs = new TiXmlElement("node");
			TiXmlElement* fbcb2_inc = new TiXmlElement("link");
			TiXmlElement* eplrs_rs_inc = new TiXmlElement("link");

			for (TiXmlElement *info = data->FirstChildElement(); info != NULL; info = info->NextSiblingElement()){
				if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "inc_model") == 0){//进行inc处理
					Plane->LinkEndChild(inc);
					inc->SetAttribute("name", "inc");
					inc->SetAttribute("model", "INC");
					inc->SetAttribute("ignore_questions", "true");
					inc->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					inc->LinkEndChild(x_position);
					inc->LinkEndChild(y_position);
					inc->LinkEndChild(id);
					cout << "Plane--inc" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "eplrs_rs_model") == 0){//进行eplrs_rs处理
					Plane->LinkEndChild(eplrs_rs);
					eplrs_rs->SetAttribute("name", "eplrs_rs");
					eplrs_rs->SetAttribute("model", "eplrs_rs");
					eplrs_rs->SetAttribute("ignore_questions", "true");
					eplrs_rs->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "1.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					eplrs_rs->LinkEndChild(x_position);
					eplrs_rs->LinkEndChild(y_position);
					eplrs_rs->LinkEndChild(id);
					cout << "Plane--eplrs_rs" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "fbcb2_model") == 0){//进行fbcb2处理
					Plane->LinkEndChild(fbcb2);
					fbcb2->SetAttribute("name", "fbcb2");
					fbcb2->SetAttribute("model", "fbcb2");
					fbcb2->SetAttribute("ignore_questions", "true");
					fbcb2->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="600"/>
					<attr name="user id" value="2"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "1");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "5");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					fbcb2->LinkEndChild(x_position);
					fbcb2->LinkEndChild(y_position);
					fbcb2->LinkEndChild(id);
					cout << "Plane—fbcb2" << endl;
				}
				else if (strcmp(info->Attribute("name"), "network id") == 0){//user id设置
					/*
					根据network id设置user id
					*/
					TiXmlElement *EidAttr = info->FirstChildElement();//
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", info->Attribute("value"));
					Plane->LinkEndChild(id);
				}
				else if (strcmp(info->Attribute("name"), "father_id") == 0){//eplrs_rs设置

					/*
					eplrs 配置
					<attr name="Network Name (IF1 P0)" value="1"/>
					*/
					TiXmlElement *netid = new TiXmlElement("attr");
					netid->SetAttribute("name", "Network Name (IF1 P0)");
					netid->SetAttribute("value", info->Attribute("value"));
					string num = info->Attribute("value");
					eplrs_rs->LinkEndChild(netid);

				}
				else if (strcmp(info->Attribute("name"), "longitude") == 0){
					//经度处理
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", info->Attribute("value"));
					Plane->LinkEndChild(x_position);
				}
				else if (strcmp(info->Attribute("name"), "latitude") == 0){
					//纬度处理
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", info->Attribute("value"));
					Plane->LinkEndChild(y_position);
				}
				else if (strcmp(info->Attribute("name"), "height") == 0){
					//高度处理
					TiXmlElement *h_position = new TiXmlElement("attr");
					h_position->SetAttribute("name", "altitude");
					h_position->SetAttribute("value", info->Attribute("value"));
					Plane->LinkEndChild(h_position);
				}
				else
					cout << "其他Plane属性处理" << endl;
			}


			/*
			链路配置
			*/
			/*
			fbcb2_inc
			<link name="fbcb2-inc" model="100BaseT" class="duplex" srcNode="inc" destNode="fbcb2" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="inc.eth_tx_1_0"/>
			<attr name="receiver a" value="inc.eth_rx_1_0"/>
			<attr name="transmitter b" value="fbcb2.eth_tx_0_0"/>
			<attr name="receiver b" value="fbcb2.eth_rx_0_0"/>

			*/
			Plane->LinkEndChild(fbcb2_inc);
			fbcb2_inc->SetAttribute("name", "fbcb2_inc");
			fbcb2_inc->SetAttribute("model", "100BaseT");
			fbcb2_inc->SetAttribute("class", "duplex");
			fbcb2_inc->SetAttribute("srcNode", "inc");
			fbcb2_inc->SetAttribute("destNode", "fbcb2");
			fbcb2_inc->SetAttribute("ignore_questions", "true");
			fbcb2_inc->SetAttribute("min_match_score", "strict matching");
			TiXmlElement *a1 = new TiXmlElement("attr");
			TiXmlElement *a2 = new TiXmlElement("attr");
			TiXmlElement *b1 = new TiXmlElement("attr");
			TiXmlElement *b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "fbcb2.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "fbcb2.eth_rx_0_0");
			fbcb2_inc->LinkEndChild(a1);
			fbcb2_inc->LinkEndChild(a2);
			fbcb2_inc->LinkEndChild(b1);
			fbcb2_inc->LinkEndChild(b2);

			/*
			eplrs_rs_inc
			<link name="eplrs_rs-inc" model="100BaseT" class="duplex" srcNode="eplrs_rs" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="eplrs_rs.eth_tx_0_0"/>
			<attr name="receiver a" value="eplrs_rs.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_2_0"/>
			<attr name="receiver b" value="inc.eth_rx_2_0"/>
			*/
			Plane->LinkEndChild(eplrs_rs_inc);
			eplrs_rs_inc->SetAttribute("name", "eplrs_rs_inc");
			eplrs_rs_inc->SetAttribute("model", "100BaseT");
			eplrs_rs_inc->SetAttribute("class", "duplex");
			eplrs_rs_inc->SetAttribute("srcNode", "eplrs_rs");
			eplrs_rs_inc->SetAttribute("destNode", "inc");
			eplrs_rs_inc->SetAttribute("ignore_questions", "true");
			eplrs_rs_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "eplrs_rs.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "eplrs_rs.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_1_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_1_0");
			eplrs_rs_inc->LinkEndChild(a1);
			eplrs_rs_inc->LinkEndChild(a2);
			eplrs_rs_inc->LinkEndChild(b1);
			eplrs_rs_inc->LinkEndChild(b2);
		}
		else if (strcmp(data->Attribute("model"), "Ship") == 0){//字符串比较，对Ship子网进行处理
			cout << "处理Ship" << endl;
			TiXmlElement* Ship = new TiXmlElement("subnet");
			network->LinkEndChild(Ship);//从属于network
			//进行相关属性设置,<subnet name="JS_Command1" mobility="mobile">
			Ship->SetAttribute("name", data->Attribute("name"));
			Ship->SetAttribute("mobility", "mobile");
			//然后依次进行eplrs_rs转换、inc转换、fbcb2转换。然后进行链路eplrs_rs_inc转换、链路fbcb2-inc转换。
			//<node name="mse_nc" model="mse_nc" ignore_questions="true" min_match_score="strict matching">
			/*
			<attr name="network id" value="1"/>
			<attr name="father_id" value="1"/>
			<attr name="longitude" value="0"/>
			<attr name="latitude" value="0"/>
			<attr name="height" value="0"/>
			*/
			TiXmlElement* fbcb2 = new TiXmlElement("node");
			TiXmlElement* inc = new TiXmlElement("node");
			TiXmlElement* eplrs_rs = new TiXmlElement("node");
			TiXmlElement* fbcb2_inc = new TiXmlElement("link");
			TiXmlElement* eplrs_rs_inc = new TiXmlElement("link");

			for (TiXmlElement *info = data->FirstChildElement(); info != NULL; info = info->NextSiblingElement()){
				if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "inc_model") == 0){//进行inc处理
					Ship->LinkEndChild(inc);
					inc->SetAttribute("name", "inc");
					inc->SetAttribute("model", "INC");
					inc->SetAttribute("ignore_questions", "true");
					inc->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					inc->LinkEndChild(x_position);
					inc->LinkEndChild(y_position);
					inc->LinkEndChild(id);
					cout << "Ship--inc" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "eplrs_rs_model") == 0){//进行eplrs_rs处理
					Ship->LinkEndChild(eplrs_rs);
					eplrs_rs->SetAttribute("name", "eplrs_rs");
					eplrs_rs->SetAttribute("model", "eplrs_rs");
					eplrs_rs->SetAttribute("ignore_questions", "true");
					eplrs_rs->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "1.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					eplrs_rs->LinkEndChild(x_position);
					eplrs_rs->LinkEndChild(y_position);
					eplrs_rs->LinkEndChild(id);
					cout << "Ship--eplrs_rs" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "fbcb2_model") == 0){//进行fbcb2处理
					Ship->LinkEndChild(fbcb2);
					fbcb2->SetAttribute("name", "fbcb2");
					fbcb2->SetAttribute("model", "fbcb2");
					fbcb2->SetAttribute("ignore_questions", "true");
					fbcb2->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="600"/>
					<attr name="user id" value="2"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "1");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "5");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					fbcb2->LinkEndChild(x_position);
					fbcb2->LinkEndChild(y_position);
					fbcb2->LinkEndChild(id);
					cout << "Ship—fbcb2" << endl;
				}
				else if (strcmp(info->Attribute("name"), "network id") == 0){//user id设置
					/*
					根据network id设置user id
					*/
					TiXmlElement *EidAttr = info->FirstChildElement();//
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", info->Attribute("value"));
					Ship->LinkEndChild(id);
				}
				else if (strcmp(info->Attribute("name"), "father_id") == 0){//eplrs_rs设置

					/*
					eplrs 配置
					<attr name="Network Name (IF1 P0)" value="1"/>
					*/
					TiXmlElement *netid = new TiXmlElement("attr");
					netid->SetAttribute("name", "Network Name (IF1 P0)");
					netid->SetAttribute("value", info->Attribute("value"));
					string num = info->Attribute("value");
					eplrs_rs->LinkEndChild(netid);

				}
				else if (strcmp(info->Attribute("name"), "longitude") == 0){
					//经度处理
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", info->Attribute("value"));
					Ship->LinkEndChild(x_position);
				}
				else if (strcmp(info->Attribute("name"), "latitude") == 0){
					//纬度处理
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", info->Attribute("value"));
					Ship->LinkEndChild(y_position);
				}
				else if (strcmp(info->Attribute("name"), "height") == 0){
					//高度处理
					TiXmlElement *h_position = new TiXmlElement("attr");
					h_position->SetAttribute("name", "altitude");
					h_position->SetAttribute("value", info->Attribute("value"));
					Ship->LinkEndChild(h_position);
				}
				else
					cout << "其他Ship属性处理" << endl;
			}


			/*
			链路配置
			*/
			/*
			fbcb2_inc
			<link name="fbcb2-inc" model="100BaseT" class="duplex" srcNode="inc" destNode="fbcb2" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="inc.eth_tx_1_0"/>
			<attr name="receiver a" value="inc.eth_rx_1_0"/>
			<attr name="transmitter b" value="fbcb2.eth_tx_0_0"/>
			<attr name="receiver b" value="fbcb2.eth_rx_0_0"/>

			*/
			Ship->LinkEndChild(fbcb2_inc);
			fbcb2_inc->SetAttribute("name", "fbcb2_inc");
			fbcb2_inc->SetAttribute("model", "100BaseT");
			fbcb2_inc->SetAttribute("class", "duplex");
			fbcb2_inc->SetAttribute("srcNode", "inc");
			fbcb2_inc->SetAttribute("destNode", "fbcb2");
			fbcb2_inc->SetAttribute("ignore_questions", "true");
			fbcb2_inc->SetAttribute("min_match_score", "strict matching");
			TiXmlElement *a1 = new TiXmlElement("attr");
			TiXmlElement *a2 = new TiXmlElement("attr");
			TiXmlElement *b1 = new TiXmlElement("attr");
			TiXmlElement *b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "fbcb2.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "fbcb2.eth_rx_0_0");
			fbcb2_inc->LinkEndChild(a1);
			fbcb2_inc->LinkEndChild(a2);
			fbcb2_inc->LinkEndChild(b1);
			fbcb2_inc->LinkEndChild(b2);

			/*
			eplrs_rs_inc
			<link name="eplrs_rs-inc" model="100BaseT" class="duplex" srcNode="eplrs_rs" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="eplrs_rs.eth_tx_0_0"/>
			<attr name="receiver a" value="eplrs_rs.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_2_0"/>
			<attr name="receiver b" value="inc.eth_rx_2_0"/>
			*/
			Ship->LinkEndChild(eplrs_rs_inc);
			eplrs_rs_inc->SetAttribute("name", "eplrs_rs_inc");
			eplrs_rs_inc->SetAttribute("model", "100BaseT");
			eplrs_rs_inc->SetAttribute("class", "duplex");
			eplrs_rs_inc->SetAttribute("srcNode", "eplrs_rs");
			eplrs_rs_inc->SetAttribute("destNode", "inc");
			eplrs_rs_inc->SetAttribute("ignore_questions", "true");
			eplrs_rs_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "eplrs_rs.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "eplrs_rs.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_1_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_1_0");
			eplrs_rs_inc->LinkEndChild(a1);
			eplrs_rs_inc->LinkEndChild(a2);
			eplrs_rs_inc->LinkEndChild(b1);
			eplrs_rs_inc->LinkEndChild(b2);
		}
		else if (strcmp(data->Attribute("model"), "Car") == 0){//字符串比较，对Car子网进行处理
			cout << "处理Car" << endl;
			TiXmlElement* Car = new TiXmlElement("subnet");
			network->LinkEndChild(Car);//从属于network
			//进行相关属性设置,<subnet name="JS_Command1" mobility="mobile">
			Car->SetAttribute("name", data->Attribute("name"));
			Car->SetAttribute("mobility", "mobile");
			//然后依次进行eplrs_rs转换、inc转换、fbcb2转换。然后进行链路eplrs_rs_inc转换、链路fbcb2-inc转换。
			//<node name="mse_nc" model="mse_nc" ignore_questions="true" min_match_score="strict matching">
			/*
			<attr name="network id" value="1"/>
			<attr name="father_id" value="1"/>
			<attr name="longitude" value="0"/>
			<attr name="latitude" value="0"/>
			<attr name="height" value="0"/>
			*/
			TiXmlElement* fbcb2 = new TiXmlElement("node");
			TiXmlElement* inc = new TiXmlElement("node");
			TiXmlElement* eplrs_rs = new TiXmlElement("node");
			TiXmlElement* fbcb2_inc = new TiXmlElement("link");
			TiXmlElement* eplrs_rs_inc = new TiXmlElement("link");

			for (TiXmlElement *info = data->FirstChildElement(); info != NULL; info = info->NextSiblingElement()){
				if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "inc_model") == 0){//进行inc处理
					Car->LinkEndChild(inc);
					inc->SetAttribute("name", "inc");
					inc->SetAttribute("model", "INC");
					inc->SetAttribute("ignore_questions", "true");
					inc->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "0.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					inc->LinkEndChild(x_position);
					inc->LinkEndChild(y_position);
					inc->LinkEndChild(id);
					cout << "Car--inc" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "eplrs_rs_model") == 0){//进行eplrs_rs处理
					Car->LinkEndChild(eplrs_rs);
					eplrs_rs->SetAttribute("name", "eplrs_rs");
					eplrs_rs->SetAttribute("model", "eplrs_rs");
					eplrs_rs->SetAttribute("ignore_questions", "true");
					eplrs_rs->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="300"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "0.0");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "1.0");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					eplrs_rs->LinkEndChild(x_position);
					eplrs_rs->LinkEndChild(y_position);
					eplrs_rs->LinkEndChild(id);
					cout << "Car--eplrs_rs" << endl;
				}
				else if (info->Attribute("model") != NULL&&strcmp(info->Attribute("model"), "fbcb2_model") == 0){//进行fbcb2处理
					Car->LinkEndChild(fbcb2);
					fbcb2->SetAttribute("name", "fbcb2");
					fbcb2->SetAttribute("model", "fbcb2");
					fbcb2->SetAttribute("ignore_questions", "true");
					fbcb2->SetAttribute("min_match_score", "strict matching");
					/*
					<attr name="x position" value="0.0"/>
					<attr name="y position" value="600"/>
					<attr name="user id" value="2"/>
					*/
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", "1");
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", "5");

					TiXmlElement *EidAttr = info->FirstChildElement();//equid id转化为 user id
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", EidAttr->Attribute("value"));

					fbcb2->LinkEndChild(x_position);
					fbcb2->LinkEndChild(y_position);
					fbcb2->LinkEndChild(id);
					cout << "Car—fbcb2" << endl;
				}
				else if (strcmp(info->Attribute("name"), "network id") == 0){//user id设置
					/*
					根据network id设置user id
					*/
					TiXmlElement *EidAttr = info->FirstChildElement();//
					TiXmlElement *id = new TiXmlElement("attr");
					id->SetAttribute("name", "user id");
					id->SetAttribute("value", info->Attribute("value"));
					Car->LinkEndChild(id);
				}
				else if (strcmp(info->Attribute("name"), "father_id") == 0){//eplrs_rs设置

					/*
					eplrs 配置
					<attr name="Network Name (IF1 P0)" value="1"/>
					*/
					TiXmlElement *netid = new TiXmlElement("attr");
					netid->SetAttribute("name", "Network Name (IF1 P0)");
					netid->SetAttribute("value", info->Attribute("value"));
					string num = info->Attribute("value");
					eplrs_rs->LinkEndChild(netid);

				}
				else if (strcmp(info->Attribute("name"), "longitude") == 0){
					//经度处理
					TiXmlElement *x_position = new TiXmlElement("attr");
					x_position->SetAttribute("name", "x position");
					x_position->SetAttribute("value", info->Attribute("value"));
					Car->LinkEndChild(x_position);
				}
				else if (strcmp(info->Attribute("name"), "latitude") == 0){
					//纬度处理
					TiXmlElement *y_position = new TiXmlElement("attr");
					y_position->SetAttribute("name", "y position");
					y_position->SetAttribute("value", info->Attribute("value"));
					Car->LinkEndChild(y_position);
				}
				else if (strcmp(info->Attribute("name"), "height") == 0){
					//高度处理
					TiXmlElement *h_position = new TiXmlElement("attr");
					h_position->SetAttribute("name", "altitude");
					h_position->SetAttribute("value", info->Attribute("value"));
					Car->LinkEndChild(h_position);
				}
				else
					cout << "其他Car属性处理" << endl;
			}
			/*
			链路配置
			*/
			/*
			fbcb2_inc
			<link name="fbcb2-inc" model="100BaseT" class="duplex" srcNode="inc" destNode="fbcb2" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="inc.eth_tx_1_0"/>
			<attr name="receiver a" value="inc.eth_rx_1_0"/>
			<attr name="transmitter b" value="fbcb2.eth_tx_0_0"/>
			<attr name="receiver b" value="fbcb2.eth_rx_0_0"/>

			*/
			Car->LinkEndChild(fbcb2_inc);
			fbcb2_inc->SetAttribute("name", "fbcb2_inc");
			fbcb2_inc->SetAttribute("model", "100BaseT");
			fbcb2_inc->SetAttribute("class", "duplex");
			fbcb2_inc->SetAttribute("srcNode", "inc");
			fbcb2_inc->SetAttribute("destNode", "fbcb2");
			fbcb2_inc->SetAttribute("ignore_questions", "true");
			fbcb2_inc->SetAttribute("min_match_score", "strict matching");
			TiXmlElement *a1 = new TiXmlElement("attr");
			TiXmlElement *a2 = new TiXmlElement("attr");
			TiXmlElement *b1 = new TiXmlElement("attr");
			TiXmlElement *b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "inc.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "inc.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "fbcb2.eth_tx_0_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "fbcb2.eth_rx_0_0");
			fbcb2_inc->LinkEndChild(a1);
			fbcb2_inc->LinkEndChild(a2);
			fbcb2_inc->LinkEndChild(b1);
			fbcb2_inc->LinkEndChild(b2);

			/*
			eplrs_rs_inc
			<link name="eplrs_rs-inc" model="100BaseT" class="duplex" srcNode="eplrs_rs" destNode="inc" ignore_questions="true" min_match_score="strict matching">
			<attr name="transmitter a" value="eplrs_rs.eth_tx_0_0"/>
			<attr name="receiver a" value="eplrs_rs.eth_rx_0_0"/>
			<attr name="transmitter b" value="inc.eth_tx_2_0"/>
			<attr name="receiver b" value="inc.eth_rx_2_0"/>
			*/
			Car->LinkEndChild(eplrs_rs_inc);
			eplrs_rs_inc->SetAttribute("name", "eplrs_rs_inc");
			eplrs_rs_inc->SetAttribute("model", "100BaseT");
			eplrs_rs_inc->SetAttribute("class", "duplex");
			eplrs_rs_inc->SetAttribute("srcNode", "eplrs_rs");
			eplrs_rs_inc->SetAttribute("destNode", "inc");
			eplrs_rs_inc->SetAttribute("ignore_questions", "true");
			eplrs_rs_inc->SetAttribute("min_match_score", "strict matching");
			a1 = new TiXmlElement("attr");
			a2 = new TiXmlElement("attr");
			b1 = new TiXmlElement("attr");
			b2 = new TiXmlElement("attr");
			a1->SetAttribute("name", "transmitter a");
			a1->SetAttribute("value", "eplrs_rs.eth_tx_0_0");
			a2->SetAttribute("name", "receiver a");
			a2->SetAttribute("value", "eplrs_rs.eth_rx_0_0");
			b1->SetAttribute("name", "transmitter b");
			b1->SetAttribute("value", "inc.eth_tx_1_0");
			b2->SetAttribute("name", "receiver b");
			b2->SetAttribute("value", "inc.eth_rx_1_0");
			eplrs_rs_inc->LinkEndChild(a1);
			eplrs_rs_inc->LinkEndChild(a2);
			eplrs_rs_inc->LinkEndChild(b1);
			eplrs_rs_inc->LinkEndChild(b2);
		}
	}

	//配置TDMAconfig
	/*
	<node name="eplrs_config" model="tdma_config" ignore_questions="true" min_match_score="strict matching">
	<attr name="x position" value="795.7"/>
	<attr name="y position" value="-98.96"/>
	<attr name="threshold" value="0.0"/>
	<attr name="icon name" value="util_tdmaconfig"/>
	<attr name="doc file" value=""/>
	<attr name="tooltip" value=""/>
	<attr name="creation source" value="Object copy"/>
	<attr name="creation timestamp" value="10:42:32 Jun 29 2018"/>
	<attr name="creation data" value="Copy of node_0"/>
	<attr name="label color" value="black"/>
	<attr name="simple icon name" value=""/>
	<attr name="Network Definitions.count" value="2"/>
	<attr name="Network Definitions [0].Network Name" value="1"/>
	<attr name="Network Definitions [0].TDMA Profile" value="Default" symbolic="true"/>
	<attr name="Network Definitions [0].Channel Settings.count" value="1"/>
	<attr name="Network Definitions [1].Network Name" value="2"/>
	<attr name="Network Definitions [1].TDMA Profile" value="Default" symbolic="true"/>
	<attr name="Network Definitions [1].Channel Settings.count" value="1"/>
	</node>
	*/
	eplrs_config->SetAttribute("name", "eplrs_config");
	eplrs_config->SetAttribute("model", "tdma_config");
	eplrs_config->SetAttribute("ignore_questions", "true");
	eplrs_config->SetAttribute("min_match_score", "strict matching");
	TiXmlElement *x_position = new TiXmlElement("attr");
	x_position->SetAttribute("name", "x position");
	x_position->SetAttribute("value", "10.0");
	TiXmlElement *y_position = new TiXmlElement("attr");
	y_position->SetAttribute("name", "y position");
	y_position->SetAttribute("value", "10.0");
	eplrs_config->LinkEndChild(x_position);
	eplrs_config->LinkEndChild(y_position);

	TiXmlElement *netDef = new TiXmlElement("attr");
	netDef->SetAttribute("name", "Network Definitions.count");
	netDef->SetAttribute("value", lvId.size());
	eplrs_config->LinkEndChild(netDef);
	for (int i = 0; i < lvId.size(); i++){
		/*
		<attr name="Network Definitions [0].Network Name" value="1"/>
		<attr name="Network Definitions [0].TDMA Profile" value="Default" symbolic="true"/>
		<attr name="Network Definitions [0].Channel Settings.count" value="1"/>
		*/
		TiXmlElement *d1 = new TiXmlElement("attr");
		TiXmlElement *d2 = new TiXmlElement("attr");
		TiXmlElement *d3 = new TiXmlElement("attr");
		d1->SetAttribute("name", ("Network Definitions [" + to_string(i) + "].Network Name").data());
		d1->SetAttribute("value", lvId[i].data());
		d2->SetAttribute("name", ("Network Definitions [" + to_string(i) + "].TDMA Profile").data());
		d2->SetAttribute("value", "Default");
		d2->SetAttribute("symbolic", "true");
		d3->SetAttribute("name", ("Network Definitions [" + to_string(i) + "].Channel Settings.count").data());
		d3->SetAttribute("value", "1");
		eplrs_config->LinkEndChild(d1);
		eplrs_config->LinkEndChild(d2);
		eplrs_config->LinkEndChild(d3);

	}
	network->LinkEndChild(eplrs_config);
	
	//hla模块
	TiXmlElement *hla1 = new TiXmlElement("node");
	hla1->SetAttribute("name", "HLA13");
	hla1->SetAttribute("model", "rpr_hla13");
	hla1->SetAttribute("ignore_questions", "true");
	hla1->SetAttribute("min_match_score", "strict matching");

	TiXmlElement *x1 = new TiXmlElement("attr");
	TiXmlElement *y1 = new TiXmlElement("attr");
	x1->SetAttribute("name", "x position");
	x1->SetAttribute("value", "116");
	y1->SetAttribute("name", "y position");
	y1->SetAttribute("value", "39");
	hla1->LinkEndChild(x1);
	hla1->LinkEndChild(y1);

	TiXmlElement *hla2 = new TiXmlElement("node");
	TiXmlElement *x2 = new TiXmlElement("attr");
	TiXmlElement *y2 = new TiXmlElement("attr");
	hla2->SetAttribute("name", "hla_13");
	hla2->SetAttribute("model", "hla_13");
	hla2->SetAttribute("ignore_questions", "true");
	hla2->SetAttribute("min_match_score", "strict matching");
	x2->SetAttribute("name", "x position");
	x2->SetAttribute("value", "116");
	y2->SetAttribute("name", "y position");
	y2->SetAttribute("value", "50");
	hla2->LinkEndChild(x2);
	hla2->LinkEndChild(y2);

	network->LinkEndChild(hla1);
	network->LinkEndChild(hla2);

	//添加sitl，一个放在旅，一个放在营。
	/*
		<node name = "node_2" model = "sitl_virtual_gateway_to_real_world" ignore_questions = "true" min_match_score = "strict matching">
			<attr name = "x position" value = "5" / >
			<attr name = "y position" value = "5" / >
			<attr name = "threshold" value = "0.0" / >
			<attr name = "icon name" value = "SITL_gateway" / >
			<attr name = "doc file" value = "" / >
			<attr name = "tooltip" value = "" / >
			<attr name = "creation source" value = "Object Palette" / >
			<attr name = "creation timestamp" value = "15:18:14 Jun 28 2018" / >
			<attr name = "creation data" value = "" / >
			<attr name = "label color" value = "black" / >
			<attr name = "simple icon name" value = "" / >
			<attr name = "Network Adapter" value = "Intel(R) PRO/1000 MT Network Connection MAC: 00:0c:29:73:0f:99 IPv4: 192.168.2.2 IPv6: fe80::5c69:f7f9:a363:9433%18" / >
			<attr name = "Network Adapter Name" value = "\Device\WPRO_41_1742_{940620BE-FD66-4F36-A6C6-41AB2BFF3DD2}" / >
			<attr name=  "Network Adapter Name" value = "\Device\WPRO_41_1742_{940620BE-FD66-4F36-A6C6-41AB2BFF3DD2}"/>
		< / node>
	*/
	/*

	TiXmlElement *sitl1 = new TiXmlElement("node");
	sitl1->SetAttribute("name", "sitl");
	sitl1->SetAttribute("model", "sitl_virtual_gateway_to_real_world");
	sitl1->SetAttribute("ignore_questions", "true");
	sitl1->SetAttribute("min_match_score", "strict matching");

	TiXmlElement *x1 = new TiXmlElement("attr");
	TiXmlElement *y1 = new TiXmlElement("attr");
	TiXmlElement *nav1 = new TiXmlElement("attr");
	TiXmlElement *nan1 = new TiXmlElement("attr");

	x1->SetAttribute("name", "x position");
	x1->SetAttribute("value", "5");
	y1->SetAttribute("name", "y position");
	y1->SetAttribute("value", "5");
	nav1->SetAttribute("name", "Network Adapter");
	nav1->SetAttribute("value", "Intel(R) PRO/1000 MT Network Connection MAC: 00:0c:29:73:0f:99 IPv4: 192.168.2.2 IPv6: fe80::5c69:f7f9:a363:9433%18");
	nan1->SetAttribute("name", "Network Adapter Name");
	nan1->SetAttribute("value", "\\Device\\WPRO_41_1742_{940620BE-FD66-4F36-A6C6-41AB2BFF3DD2}");
	sitl1->LinkEndChild(x1);
	sitl1->LinkEndChild(y1);
	sitl1->LinkEndChild(nav1);
	sitl1->LinkEndChild(nan1);
	sitl_L->LinkEndChild(sitl1);

	TiXmlElement *sitl_inc = new TiXmlElement("link");
	sitl_L->LinkEndChild(sitl_inc);
	sitl_inc->SetAttribute("name", "sitl_inc");
	sitl_inc->SetAttribute("model", "sitl_virtual_eth_link");
	sitl_inc->SetAttribute("class", "duplex");
	sitl_inc->SetAttribute("srcNode", "sitl");
	sitl_inc->SetAttribute("destNode", "inc");
	sitl_inc->SetAttribute("ignore_questions", "true");
	sitl_inc->SetAttribute("min_match_score", "strict matching");
	TiXmlElement *a1 = new TiXmlElement("attr");
	TiXmlElement *a2 = new TiXmlElement("attr");
	TiXmlElement *b1 = new TiXmlElement("attr");
	TiXmlElement *b2 = new TiXmlElement("attr");
	a1->SetAttribute("name", "transmitter a");
	a1->SetAttribute("value", "sitl.Pt-to-Pt Tx");
	a2->SetAttribute("name", "receiver a");
	a2->SetAttribute("value", "sitl.Pt-to-Pt Rx");
	b1->SetAttribute("name", "transmitter b");
	b1->SetAttribute("value", "inc.eth_tx_5_0");
	b2->SetAttribute("name", "receiver b");
	b2->SetAttribute("value", "inc.eth_rx_5_0");
	sitl_inc->LinkEndChild(a1);
	sitl_inc->LinkEndChild(a2);
	sitl_inc->LinkEndChild(b1);
	sitl_inc->LinkEndChild(b2);

	*/



	writeDoc->SaveFile("dest.xml");
	delete writeDoc;
	cout << "ok!" << endl;
	return 0;
}


