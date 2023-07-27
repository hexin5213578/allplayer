#!/usr/bin/python
# -*- coding: UTF-8 -*-
import ftplib
from ftplib import FTP
import os
import sys
import time
import socket
import argparse
import shutil


# 使用说明：运行本运行脚本，可以将生成的文件归档到archive路径下，然后上传到ftp服务器上，文件夹自动加上V前缀。
# 参数说明： 
#     --version 版本号  例如：4.3.1


class MyFTP:
    def __init__(self, host, port=21):
        """ 初始化 FTP 客户端
        参数:
                 host:ip地址

                 port:端口号
        """
        # print("__init__()---> host = %s ,port = %s" % (host, port))

        self.host = host
        self.port = port
        self.ftp = FTP()
        # 重新设置下编码方式
        # self.ftp.encoding = 'gbk'

        self.log_file = open("log.txt", "a")
        self.file_list = []

    def login(self, username, password):
        """ 初始化 FTP 客户端
            参数:
                username: 用户名
                password: 密码
            """
        try:
            timeout = 60
            socket.setdefaulttimeout(timeout)
            # 0主动模式 1 #被动模式
            # self.ftp.set_pasv(True)
            # 打开调试级别2，显示详细信息
            # self.ftp.set_debuglevel(2)

            self.debug_print('开始尝试连接到 %s' % self.host)
            self.ftp.connect(self.host, self.port)
            self.debug_print('成功连接到 %s' % self.host)

            self.debug_print('开始尝试登录到 %s' % self.host)
            self.ftp.login(username, password)
            self.debug_print('成功登录到 %s' % self.host)

            self.debug_print(self.ftp.welcome)
        except Exception as err:
            self.deal_error("FTP 连接或登录失败 ，错误描述为：%s" % err)
            pass

    def is_same_size(self, local_file, remote_file):
        """判断远程文件和本地文件大小是否一致
           参数:
             local_file: 本地文件
             remote_file: 远程文件
        """
        try:
            remote_file_size = self.ftp.size(remote_file)
        except Exception as err:
            # self.debug_print("is_same_size() 错误描述为：%s" % err)
            remote_file_size = -1

        try:
            local_file_size = os.path.getsize(local_file)
        except Exception as err:
            # self.debug_print("is_same_size() 错误描述为：%s" % err)
            local_file_size = -1

        #self.debug_print('local_file_size:%d  , remote_file_size:%d' % (local_file_size, remote_file_size))
        if remote_file_size == local_file_size:
            return 1
        else:
            return 0

    def download_file(self, local_file, remote_file):
        """从ftp下载文件
            参数:
                local_file: 本地文件

                remote_file: 远程文件
        """
        self.debug_print("download_file()---> local_path = %s ,remote_path = %s" % (local_file, remote_file))

        if self.is_same_size(local_file, remote_file):
            self.debug_print('%s 文件大小相同，无需下载' % local_file)
            return
        else:
            try:
                self.debug_print('>>>>>>>>>>>>下载文件 %s ... ...' % local_file)
                buf_size = 1024
                file_handler = open(local_file, 'wb')
                self.ftp.retrbinary('RETR %s' % remote_file, file_handler.write, buf_size)
                file_handler.close()
            except Exception as err:
                self.debug_print('下载文件出错，出现异常：%s ' % err)
                return

    def download_file_tree(self, local_path, remote_path):
        """从远程目录下载多个文件到本地目录
                       参数:
                        local_path: 本地路径
                        remote_path: 远程路径
                """
        print("download_file_tree()--->  local_path = %s ,remote_path = %s" % (local_path, remote_path))
        try:
            self.ftp.cwd(remote_path)
        except Exception as err:
            self.debug_print('远程目录%s不存在，继续...' % remote_path + " ,具体错误描述为：%s" % err)
            return

        if not os.path.isdir(local_path):
            self.debug_print('本地目录%s不存在，先创建本地目录' % local_path)
            os.makedirs(local_path)

        self.debug_print('切换至目录: %s' % self.ftp.pwd())

        self.file_list = []
        # 方法回调
        self.ftp.dir(self.get_file_list)

        remote_names = self.file_list
        self.debug_print('远程目录 列表: %s' % remote_names)
        for item in remote_names:
            file_type = item[0]
            file_name = item[1]
            local = os.path.join(local_path, file_name)
            if file_type == 'd':
                print("download_file_tree()---> 下载目录： %s" % file_name)
                self.download_file_tree(local, file_name)
            elif file_type == '-':
                print("download_file()---> 下载文件： %s" % file_name)
                self.download_file(local, file_name)
            self.ftp.cwd("..")
            self.debug_print('返回上层目录 %s' % self.ftp.pwd())
        return True

    def upload_file(self, local_file, remote_file):
        """从本地上传文件到ftp
           参数:
             local_path: 本地文件
             remote_path: 远程文件
        """
        if not os.path.isfile(local_file):
            self.debug_print('%s 不存在' % local_file)
            return

        if self.is_same_size(local_file, remote_file):
            self.debug_print('跳过相等的文件: %s' % local_file)
            return

        buf_size = 1024
        file_handler = open(local_file, 'rb')
        self.ftp.storbinary('STOR %s' % remote_file, file_handler, buf_size)
        file_handler.close()
        self.debug_print('上传: %s' % local_file + "成功!")

    # 递归删除远程文件夹
    def delete_folder(self, folder_path):
        # 切换到文件夹的目标目录
        self.ftp.cwd(folder_path)

        # 获取当前工作目录下的所有文件和文件夹
        files_and_folders = self.ftp.nlst()

        # 遍历文件和文件夹
        for file_or_folder in files_and_folders:
            # 获取文件的完整路径
            file_path = os.path.join(self.ftp.pwd(), file_or_folder)
            try:
                # 如果能切换到文件夹，就是文件夹
                self.ftp.cwd(file_or_folder)
                self.ftp.cwd("..")
                # 递归删除文件夹
                self.delete_folder(file_or_folder)
            except:
                # 如果不能切换到文件夹，就是文件
                self.ftp.delete(file_or_folder)
                pass

        # 切换回原来的工作目录
        self.ftp.cwd("..")
        # 删除文件夹
        self.ftp.rmd(folder_path)

    def upload_file_tree(self, local_path, remote_path):
        """从本地上传目录下多个文件到ftp
           参数:
             local_path: 本地路径
             remote_path: 远程路径
        """
        if not os.path.isdir(local_path):
            self.debug_print('本地目录 %s 不存在' % local_path)
            return

        dir_name, file_name = os.path.split(remote_path)
        self.ftp.cwd(dir_name)
        #  如果文件夹存在，删除文件夹
        if file_name in self.ftp.nlst():
            self.delete_folder(file_name)

        try:
            self.ftp.mkd(file_name)
        except Exception as err:
                self.debug_print("目录已存在 %s ,具体错误描述为：%s" % (remote_path, err))
        self.ftp.cwd(file_name)

        self.debug_print('切换至远程目录: %s' % self.ftp.pwd())

        local_name_list = os.listdir(local_path)
        for local_name in local_name_list:
            src = os.path.join(local_path, local_name)
            if os.path.isdir(src):
                try:
                    self.ftp.mkd(local_name)
                except Exception as err:
                    self.debug_print("目录已存在 %s ,具体错误描述为：%s" % (local_name, err))
                self.debug_print("upload_file_tree()---> 上传目录： %s" % local_name)
                self.upload_file_tree(src, local_name)
            else:
                self.debug_print("upload_file_tree()---> 上传文件： %s" % local_name)
                self.upload_file(src, local_name)
        self.ftp.cwd("..")

    def close(self):
        """ 退出ftp
        """
        self.debug_print("close()---> FTP退出")
        self.ftp.quit()
        self.log_file.close()

    def debug_print(self, s):
        """ 打印日志
        """
        self.write_log(s)

    def deal_error(self, e):
        """ 处理错误异常
            参数：
                e：异常
        """
        log_str = '发生错误: %s' % e
        self.write_log(log_str)
        sys.exit()

    def write_log(self, log_str):
        """ 记录日志
            参数：
                log_str：日志
        """
        time_now = time.localtime()
        date_now = time.strftime('%Y-%m-%d', time_now)
        format_log_str = "%s ---> %s \n " % (date_now, log_str)
        print(format_log_str)
        self.log_file.write(format_log_str)

    def get_file_list(self, line):
        """ 获取文件列表
            参数：
                line：
        """
        file_arr = self.get_file_name(line)
        # 去除  . 和  ..
        if file_arr[1] not in ['.', '..']:
            self.file_list.append(file_arr)

    def get_file_name(self, line):
        """ 获取文件名
            参数：
                line：
        """
        pos = line.rfind(':')
        while (line[pos] != ' '):
            pos += 1
        while (line[pos] == ' '):
            pos += 1
        file_arr = [line[0], line[pos:]]
        return file_arr


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="allplayer pack script command line parser")
    # 添加版本号参数
    parser.add_argument("--version", type=str, help="version info")
    args = parser.parse_args()

    if args.version == None:
        print("version is None")
        sys.exit()

    #获取当前路径
    cur_path = os.getcwd()
    print(cur_path)

    bin_path = os.path.join(cur_path, "Prj-Win/bin/")
    lib_path = os.path.join(cur_path, "Prj-Win/libs/") 
    dest_path = os.path.join(cur_path, "archive/")
    os.makedirs(dest_path, exist_ok=True); #目录不存在则创建，存在则不抛出异常
    print(bin_path)
    # 使用 shutil 库的 copytree 函数拷贝文件夹
    # shutil.copytree(src_path, dest_path)

    src = os.path.join(bin_path, "x86/Release/libAllplayer.dll")
    dst = os.path.join(dest_path, "Software/PC/x86")
    os.makedirs(dst, exist_ok=True); 
    shutil.copy(src, dst)

    src = os.path.join(lib_path, "x86/libAllplayer.lib")
    dst = os.path.join(dest_path, "Software/PC/x86")
    shutil.copy(src, dst)

    src = os.path.join(bin_path, "x86/Release/libAllplayer.pdb")
    dst = os.path.join(dest_path, "Software/PC/x86")
    shutil.copy(src, dst)

    src = os.path.join(bin_path, "x64/Release/libAllplayer.dll")
    dst = os.path.join(dest_path, "Software/PC/x64")
    os.makedirs(dst, exist_ok=True);
    shutil.copy(src, dst)

    src = os.path.join(lib_path, "x64/libAllplayer.lib")
    dst = os.path.join(dest_path, "Software/PC/x64")
    shutil.copy(src, dst)

    src = os.path.join(bin_path, "x64/Release/libAllplayer.pdb")
    dst = os.path.join(dest_path, "Software/PC/x64")
    shutil.copy(src, dst)

    src = os.path.join(cur_path, "Prj-Win/libAllplayer/libAllplayer.h")
    dst = os.path.join(dest_path, "Software/PC")
    shutil.copy(src, dst)

    src = os.path.join(cur_path, "font/allplayer.ttf")
    dst = os.path.join(dest_path, "Font",)
    os.makedirs(dst, exist_ok=True);
    shutil.copy(src, dst)

    src = os.path.join(cur_path, "README.md")
    dst = os.path.join(dest_path, "Doc")  
    os.makedirs(dst, exist_ok=True);  
    shutil.copy(src, dst)

    host_address = "172.16.20.61"
    username = "songgan"
    password = "Songgan@2021"
    my_ftp = MyFTP(host_address)
    my_ftp.login(username, password)

    upload_path = "/ACS/行业项目/05.正式发布归档目录/999.奥看云服务平台公共能力部件/AllPlayer/V%s" % args.version
    print("upload_path: %s" % upload_path)

    # 上传目录
    my_ftp.upload_file_tree(dest_path, upload_path)
    my_ftp.close()