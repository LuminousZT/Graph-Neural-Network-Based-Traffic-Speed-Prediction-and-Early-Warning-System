from models.layers import *
from os.path import join as pjoin
import tensorflow._api.v2.compat.v1 as tf
tf.disable_v2_behavior()

def build_model(inputs, n_his, Ks, Kt, blocks):
    # 构建基础模型
    x = inputs[:, 0:n_his, :, :]

    # Ko>0: 输出层时间卷积的核大小
    Ko = n_his
    # 时空卷积块
    for i, channels in enumerate(blocks):
        x = st_conv_block(x, Ks, Kt, channels, i, act_func='GLU')
        Ko -= 2 * (Kt - 1)

    # 输出层
    if Ko > 1:
        y = output_layer(x, Ko, 'output_layer')
    else:
        raise ValueError(f'错误：内核大小Ko必须大于1，但Ko为 "{Ko}".')

    tf.add_to_collection(name='copy_loss',
                         value=tf.nn.l2_loss(inputs[:, n_his - 1:n_his, :, :] - inputs[:, n_his:n_his + 1, :, :]))
    train_loss = tf.nn.l2_loss(y - inputs[:, n_his:n_his + 1, :, :])
    single_pred = y[:, 0, :, :]
    tf.add_to_collection(name='y_pred', value=single_pred)
    return train_loss, single_pred


def model_save(sess, global_steps, model_name, save_path='./output/models/'):
    # 保存已训练模型的检查点
    saver = tf.train.Saver(max_to_keep=3)
    prefix_path = saver.save(sess, pjoin(save_path, model_name), global_step=global_steps)
    print(f'<< 模型保存至 {prefix_path} ...')
