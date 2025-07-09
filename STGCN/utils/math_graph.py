import numpy as np
import pandas as pd
from scipy.sparse.linalg import eigs


def scaled_laplacian(W):
    # d ->  对角度矩阵
    n, d = np.shape(W)[0], np.sum(W, axis=1)
    # L -> 图谱拉普拉斯
    L = -W
    L[np.diag_indices_from(L)] = d
    for i in range(n):
        for j in range(n):
            if (d[i] > 0) and (d[j] > 0):
                L[i, j] = L[i, j] / np.sqrt(d[i] * d[j])
    lambda_max = eigs(L, k=1, which='LR')[0][0].real
    return np.mat(2 * L / lambda_max - np.identity(n))


def cheb_poly_approx(L, Ks, n):
    # 切比雪夫多项式近似函数
    L0, L1 = np.mat(np.identity(n)), np.mat(np.copy(L))

    if Ks > 1:
        L_list = [np.copy(L0), np.copy(L1)]
        for i in range(Ks - 2):
            Ln = np.mat(2 * L * L1 - L0)
            L_list.append(np.copy(Ln))
            L0, L1 = np.matrix(np.copy(L1)), np.matrix(np.copy(Ln))
        # L_lsit [Ks, n*n], Lk [n, Ks*n]
        return np.concatenate(L_list, axis=-1)
    elif Ks == 1:
        return np.asarray(L0)
    else:
        raise ValueError(f'错误：空间内核的大小必须大于 1，但Ks为 "{Ks}" ')


def first_approx(W, n):
    # 一阶近似函数
    A = W + np.identity(n)
    d = np.sum(A, axis=1)
    sinvD = np.sqrt(np.mat(np.diag(d)).I)
    return np.mat(sinvD * A * sinvD)


def weight_matrix(file_path, sigma2=10, epsilon=0.5, scaling=True):
    # 加载权重矩阵函数
    try:
        W = pd.read_csv(file_path, header=None).values
    except FileNotFoundError:
        print(f'错误：输入文件未在 {file_path} ')

    # 检查 W 是否为 0/1 矩阵
    if set(np.unique(W)) == {0, 1}:
        print('输入图形为 0/1 矩阵，设置 "scaling" 为 False ')
        scaling = False

    if scaling:
        n = W.shape[0]
        W = W / 10000.
        W2, W_mask = W * W, np.ones([n, n]) - np.identity(n)
        return np.exp(-W2 / sigma2) * (np.exp(-W2 / sigma2) >= epsilon) * W_mask
    else:
        return W
