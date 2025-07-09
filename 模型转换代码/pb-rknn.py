import numpy as np

from rknn.api import RKNN

if __name__ == '__main__':

    # Create RKNN object
    rknn = RKNN(verbose=True)

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=[58.5], std_values=[13.73], target_platform='rk3588')
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_tensorflow(tf_pb='./pb/out/frozen_inference_graph.pb',
                               inputs=['data_input'],
                               outputs=["strided_slice_4"],
                               input_size_list=[[1, 13, 228, 1]])
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build Model
    print('--> Building model')
    ret = rknn.build(do_quantization=True, dataset='./dataset.txt')
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export rknn model
    print('--> Export rknn model')
    ret = rknn.export_rknn('./STGCN_TensorFlow.rknn')
    if ret != 0:
        print('Export rknn model failed!')
        exit(ret)
    print('done')
    
    # 从当前路径加载模型
    # Load rknn model
    print('--> Load rknn model')
    ret = rknn.load_rknn(path='./STGCN_TensorFlow.rknn')
    if ret != 0:
        print('Load rknn model failed!')
        exit(ret)
    print('done')
    
    # Init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime()
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')
    
    # Inference
    print('--> Running model')
    # 读取.npy文件
    data = np.load('./PeMSD7_V_228.npy')
    output = rknn.inference(inputs=[data])
    
    # 将outputs转换为NumPy数组
    outputs = np.array(output)
    
    std = 13.73
    mean = 58.5
    outputs = outputs * std + mean
    
    # 获取数组维数
    num_dims = outputs.ndim

    # 打印维数
    print("Array has", num_dims, "dimensions.")
    
    # 获取数组维度大小
    dims = outputs.shape

    # 打印维度大小
    print("数组的维度大小为：", dims)

    # 打印数组的内容
    print(outputs)
    
    print('done')

    rknn.release()
