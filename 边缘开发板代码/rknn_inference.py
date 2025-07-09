import cv2
import numpy as np
import platform
import pandas as pd
import time
from flask import Flask, jsonify, request, send_file
import os
from werkzeug.utils import secure_filename

import matplotlib.pyplot as plt
from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import Qt

from rknnlite.api import RKNNLite

# 设备兼容树为RK356x/RK3576/RK3588
DEVICE_COMPATIBLE_NODE = '/proc/device-tree/compatible'

app = Flask(__name__)

# 全局变量，用于记录调用次数
global_zu_count = 0
global_shen_count = 5


@app.route('/api/upload', methods=['POST'])
def upload_file():
    global global_zu_count
    global global_shen_count

    print(request.files)  # 打印文件信息，便于调试
    # if 'file' not in request.files:
    # return jsonify({'error': 'No file part'})

    file = request.files['file']

    # if file.filename == '':
    # return jsonify({'error': 'No selected file'})

    allowed_extensions = {'txt', 'csv', 'npy', 'xlsx'}
    allowed = {'npy'}
    if file and '.' in file.filename and file.filename.rsplit('.', 1)[1].lower() in allowed_extensions:
        # print(f"if")
        filename = secure_filename(file.filename)
        upload_folder = os.getcwd()  # 使用当前工作目录作为上传文件夹
        print(f"当前工作路径: {upload_folder}")  # 打印当前工作目录
        # 嵌套的if语句
        if file and '.' in file.filename and file.filename.rsplit('.', 1)[1].lower() in allowed:
            # 读取第1行到第13行的数据
            df = pd.read_excel('updated_datas.xlsx', engine='openpyxl', header=None)

            # 转换为NumPy数组并调整形状
            data = np.reshape(df.to_numpy().astype('float16'), (21, 228))
            # print("输入数据的形状:", data.shape)
            global_shen_count += 5
            print(f"当前调用次数: {global_shen_count}")

            datas = data[0:13]

            start_time = time.time()
            output = rknn_lite.inference(inputs=[datas])
            pred_time = time.time() - start_time

            print(f'Model Pred Time {pred_time:.3f}s')
            # Show the classification results

            # 对输出进行后处理，这里示例为标准化处理
            std = 13.73
            mean = 58.5
            outputs = np.array(output) * std + mean

            # 展开outputs数组，只保留一个维度
            outputs_flat = np.reshape(outputs, (228, -1))

            # 将输出保留两位小数
            outputs_0 = [round(float(x), 1) for x in outputs_flat]

            MAPE = np.mean(np.abs(outputs_0 - datas[12]) / (datas[12] + 1e-5))

            MAE = np.mean(np.abs(outputs_0 - datas[12]))

            RMSE = np.sqrt(np.mean((outputs_0 - datas[12]) ** 2))

            print(f'MAPE:{MAPE:.3%}')

            print(f'MAE:{MAE:.3f}')

            print(f'RMSE:{RMSE:.3f}')

            # print(outputs_0)
            # print("输入数据的形状:", outputs_rounded.shape)

            # 将第2行至第12行数据移至第1行至第11行
            data[0:11] = data[1:12]

            # 将outputs_rounded数据填充至第12行
            data[11] = outputs_0

            outputs_rounded = [round(float(outputs_flat[i]), 1) for i in range(185)]
            print(outputs_rounded)

            data_orig = [round(float(data[12][i]), 1) for i in range(185)]
            print(data_orig)
            # print("输入数据的形状:", data_orig.shape)

            # 保存图像文件
            plot_path = os.path.join(upload_folder, 'output_plot.png')
            plt.figure()
            plt.plot(outputs_flat, label='Predicted')
            plt.plot(data[12], label='Original')
            plt.xlabel('Nodes')
            plt.ylabel('Speed(km/h)')
            plt.legend()
            plt.title(
                f'Group {global_zu_count}   Pred {global_shen_count} mins \n Model Pred Time:{pred_time:.3f}s \n MAPE:{MAPE:.3%}   MAE:{MAE:.3f}   RMSE:{RMSE:.3f}')
            plt.savefig(plot_path)

            # 自动打开图像文件
            os.system(f'xdg-open {plot_path}')  # 适用于 Linux

            data[12:20] = data[13:21]

            # 将更新后的 datas 数据保存到 .xlsx 文件
            output_file_path = os.path.join(upload_folder, 'updated_datas.xlsx')
            pd.DataFrame(data).to_excel(output_file_path, index=False, header=False)

            # 打印更新后的data数组
            # print(datas)

            # 将 outputs_rounded 和 data_orig 数据拼接起来
            outputs_pred = outputs_rounded + data_orig
            # print(outputs_pred)

            # 释放RKNN资源
            # rknn_lite.release()

            # 返回处理后的结果
            return jsonify({'outputs': outputs_pred})
        # print(f"if")

        global_zu_count = int(filename.split('_')[2])
        global_shen_count = 5

        file_path = os.path.join(upload_folder, filename)
        print(f"保存接收文件至: {file_path}")  # 打印文件保存路径
        file.save(file_path)

        total_time = 0

        # 读取第1行到第13行的数据
        df = pd.read_excel(file_path, engine='openpyxl', header=None)

        # 转换为NumPy数组并调整形状
        data = np.reshape(df.to_numpy().astype('float16'), (21, 228))
        # print("输入数据的形状:", data.shape)
        print(f"数据预处理");

        datas = data[0:13]
        # print("输入数据的形状:", datas.shape)

        print(f"进行推理");
        start_time = time.time()
        output = rknn_lite.inference(inputs=[datas])
        elapsed_time = time.time() - start_time
        total_time += elapsed_time

        print(f'Model Pred Time {total_time:.3f}s')

        # Show the classification results
        print(f"获取推理结果");

        # 对输出进行后处理，这里示例为标准化处理
        std = 13.73
        mean = 58.5
        outputs = np.array(output) * std + mean

        # 展开outputs数组，只保留一个维度
        outputs_flat = np.reshape(outputs, (228, -1))

        # 将输出保留两位小数
        outputs_0 = [round(float(x), 1) for x in outputs_flat]

        MAPE = np.mean(np.abs(outputs_0 - datas[12]) / (datas[12] + 1e-5))

        MAE = np.mean(np.abs(outputs_0 - datas[12]))

        RMSE = np.sqrt(np.mean((outputs_0 - datas[12]) ** 2))

        print(f'MAPE:{MAPE:.3%}')

        print(f'MAE:{MAE:.3f}')

        print(f'RMSE:{RMSE:.3f}')

        # print(outputs_0)
        # print("输入数据的形状:", outputs_rounded.shape)

        # 将第2行至第12行数据移至第1行至第11行
        data[0:11] = data[1:12]

        # 将outputs_rounded数据填充至第12行
        data[11] = outputs_0

        outputs_rounded = [round(float(outputs_flat[i]), 1) for i in range(185)]
        # print(f"Group 1---Pred 5 mins");
        print(f"预测值")
        print(outputs_rounded)

        data_orig = [round(float(data[12][i]), 1) for i in range(185)]
        print(f"真实值")
        print(data_orig)

        data[12:20] = data[13:21]

        # 将更新后的 datas 数据保存到 .xlsx 文件
        output_file_path = os.path.join(upload_folder, 'updated_datas.xlsx')
        pd.DataFrame(data).to_excel(output_file_path, index=False, header=False)

        # 打印更新后的data数组
        # print(datas)

        # print("输入数据的形状:", data_orig.shape)

        # 将 outputs_rounded 和 data_orig 数据拼接起来
        outputs_pred = outputs_rounded + data_orig
        # print(outputs_pred)

        # 释放RKNN资源
        # rknn_lite.release()

        # 保存图像文件
        plot_path = os.path.join(upload_folder, 'output_plot.png')
        plt.figure()
        plt.plot(outputs_flat, label='Predicted')
        plt.plot(data[12], label='Original')
        plt.xlabel('Nodes')
        plt.ylabel('Speed(km/h)')
        plt.legend()
        plt.title(
            f'Group {global_zu_count}   Pred {global_shen_count} mins \n Model Pred Time:{total_time:.3f}s \n MAPE:{MAPE:.3%}   MAE:{MAE:.3f}   RMSE:{RMSE:.3f}')
        plt.savefig(plot_path)

        # 自动打开图像文件
        os.system(f'xdg-open {plot_path}')  # 适用于 Linux

        # 返回处理后的结果
        print(f"返回处理后的结果");
        return jsonify({'outputs': outputs_pred})
    else:
        return jsonify({'error': 'Invalid file format'})


def get_host():
    # 获取平台和设备类型
    system = platform.system()
    machine = platform.machine()
    os_machine = system + '-' + machine
    if os_machine == 'Linux-aarch64':
        try:
            with open(DEVICE_COMPATIBLE_NODE) as f:
                device_compatible_str = f.read()
                if 'rk3588' in device_compatible_str:
                    host = 'RK3588'
                elif 'rk3562' in device_compatible_str:
                    host = 'RK3562'
                elif 'rk3576' in device_compatible_str:
                    host = 'RK3576'
                else:
                    host = 'RK3566_RK3568'
        except IOError:
            print('Read device node {} failed.'.format(DEVICE_COMPATIBLE_NODE))
            exit(-1)
    else:
        host = os_machine
    return host


RK3588_RKNN_MODEL = 'STGCN_TensorFlow.rknn'

if __name__ == '__main__':

    # 获取主机信息
    host_name = get_host()

    # 根据主机选择模型
    if host_name == 'RK3566_RK3568':
        rknn_model = RK3566_RK3568_RKNN_MODEL
    elif host_name == 'RK3562':
        rknn_model = RK3562_RKNN_MODEL
    elif host_name == 'RK3576':
        rknn_model = RK3576_RKNN_MODEL
    elif host_name == 'RK3588':
        rknn_model = RK3588_RKNN_MODEL
    else:
        print("This demo cannot run on the current platform: {}".format(host_name))
        exit(-1)

    # 初始化RKNNLite
    rknn_lite = RKNNLite()

    # 加载RKNN模型
    print('--> Load RKNN model')
    ret = rknn_lite.load_rknn(rknn_model)
    if ret != 0:
        print('Load RKNN model failed')
        exit(ret)
    print('done')

    # 初始化运行时环境
    print('--> Init runtime environment')
    if host_name in ['RK3576', 'RK3588']:
        ret = rknn_lite.init_runtime(core_mask=RKNNLite.NPU_CORE_AUTO)
    else:
        ret = rknn_lite.init_runtime()
    if ret != 0:
        print('Init runtime environment failed')
        exit(ret)
    print('done')

    # 运行模型
    print('--> Running model')

    # 启动Flask应用
    app.run(host='0.0.0.0', port=4000, debug=False)


