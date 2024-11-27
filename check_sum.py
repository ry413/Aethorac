def validate_checksum(data: str) -> bool:
    """
    验证8字节字符串的校验和（第7个字节为校验和）。
    
    参数:
        data (str): 一个由16个十六进制字符组成的字符串（8字节）。
    
    返回:
        bool: 如果校验和正确，返回 True；否则返回 False。
    """
    # 检查输入格式
    if len(data) != 16:
        print("输入必须是16个十六进制字符（8字节）")
        return False
    
    try:
        # 将输入字符串分割为每个字节
        bytes_list = [int(data[i:i + 2], 16) for i in range(0, len(data), 2)]
    except ValueError:
        print("输入包含无效的十六进制字符")
        return False

    # 提取前6个字节和校验和
    data_bytes = bytes_list[:6]
    checksum = bytes_list[6]
    expected_checksum = sum(data_bytes) & 0xFF  # 计算前6字节的校验和（低8位）

    # 打印调试信息
    print(f"数据字节: {data_bytes}")
    print(f"提供的校验和: 0x{checksum:02X}")
    print(f"计算的校验和: 0x{expected_checksum:02X}")

    # 返回校验结果
    return checksum == expected_checksum


# 测试
if __name__ == "__main__":
    # 输入一个16字符的十六进制字符串
    input_data = input("请输入一个8字节（16字符）的十六进制字符串: ").strip()
    if validate_checksum(input_data):
        print("校验和正确")
    else:
        print("校验和错误")