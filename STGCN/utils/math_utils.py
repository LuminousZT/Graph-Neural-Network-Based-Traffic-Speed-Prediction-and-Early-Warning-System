import numpy as np


def z_score(x, mean, std):
    # Z 值标准化
    return (x - mean) / std


def z_inverse(x, mean, std):
    # 函数 z_score() 的逆过程
    return x * std + mean


def MAPE(v, v_):
    # 平均绝对百分比误差
    return np.mean(np.abs(v_ - v) / (v + 1e-5))


def RMSE(v, v_):
    # 均方误差
    return np.sqrt(np.mean((v_ - v) ** 2))


def MAE(v, v_):
    # 平均绝对误差
    return np.mean(np.abs(v_ - v))


def evaluation(y, y_, x_stats):
    # 评估功能：用于计算实际与预测之间的 MAPE、MAE和RMSE
    dim = len(y_.shape)

    if dim == 3:
        # 单步
        v = z_inverse(y, x_stats['mean'], x_stats['std'])
        v_ = z_inverse(y_, x_stats['mean'], x_stats['std'])
        return np.array([MAPE(v, v_), MAE(v, v_), RMSE(v, v_)])
    else:
        # 多步
        tmp_list = []
        y = np.swapaxes(y, 0, 1)
        for i in range(y_.shape[0]):
            tmp_res = evaluation(y[i], y_[i], x_stats)
            tmp_list.append(tmp_res)
        return np.concatenate(tmp_list, axis=-1)
