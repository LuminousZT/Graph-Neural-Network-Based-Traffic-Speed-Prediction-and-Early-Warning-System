import os
import tensorflow.compat.v1 as tf
tf.disable_v2_behavior()

# import tensorflow as tf

# meta_path = 'model/Module.pd.meta'
meta_path = 'models/STGCN-9150.meta'
# 导出output_graph的路径
output_graph = "out/frozen_inference_graph.pb"

output_node_names = ["strided_slice_4","train_loss"]  # 输出节点


with tf.Session() as sess:
    # 恢复图
    saver = tf.train.import_meta_graph(meta_path, clear_devices=True)

    # 加载权重、偏置
    saver.restore(sess, tf.train.latest_checkpoint('models/'))

    # 冻结图
    frozen_graph_def = tf.graph_util.convert_variables_to_constants(
        sess,
        sess.graph_def,
        output_node_names)

    # 保存冻结图形
    with open(output_graph, 'wb') as f:
        f.write(frozen_graph_def.SerializeToString())

print("保存冻结图形")
