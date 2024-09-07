import numpy as np
import os.path
from typing import Union
import ctypes
from ctypes import c_bool as bool_t, c_ubyte as uint8_t, c_ushort as uint16_t, c_float as float_t, c_void_p
from enum import IntEnum

path = os.path.dirname(__file__)
libmipflooding = ctypes.CDLL(path + '/binaries/libmipflooding.dll')

class DATA_TYPE(IntEnum):
    UINT8 = 0
    UINT16 = 1
    FLOAT32 = 2

def get_mip_count(width: int, height: int) -> int:
    """
    Returns the number of mip levels for given resolution
    :param width: width in px
    :param height: height in px
    :return: number of mip levels
    """
    assert width < 65535
    assert height < 65535
    return libmipflooding.get_mip_count(width, height)


def get_channel_mask(channel_mask: list, channel_stride: int = 0) -> int:
    """
    Returns a channel bit mask
    :param channel_mask: List of bools or ints, 1/true = process, 0/false = ignore
    :param channel_stride: (optional) number of channels in image data, 0 = infer from list length
    :return: integer bit mask
    """
    assert len(channel_mask) <= 8
    assert len(channel_mask) <= channel_stride
    assert channel_stride <= 255

    boolean_mask = (bool_t * 8)(*channel_mask)
    for i in range(len(channel_mask)):
        boolean_mask[i] = bool_t(channel_mask[i])
    return libmipflooding.channel_mask_from_array(ctypes.pointer(boolean_mask), channel_stride)


def generate_mips(
        image: np.ndarray,
        coverage_mask: Union[np.ndarray, None] = None,
        max_threads: int = 0,
        coverage_threshold: float = 0.999,
        convert_srgb: bool = False,
        is_normal_map: bool = False,
        channel_mask: int = 0,
        scale_alpha_unweighted: bool = False,
        composite_mips: bool = True
        ) -> Union[list, None]:
    """Coverage-weighted mip maps

    Generates coverage-weighted mip maps and returns them as a list of float (0..1) NumPy arrays (excluding input image / mip 0)

    :param image: ndarray[ndim=3], types uint8 / uint16 / float, expects 0..1 range for float
    :param coverage_mask: None; ndarray[ndim=1 or 2], type uint8, linear. If None, uses last channel of image as coverage mask.
    :param max_threads: 0; Number of threads, 0 = auto (half of available threads, which amounts to number of hardware cores for machines with SMT/HyperThreading)
    :param coverage_threshold: 0.999; Threshold for mask binarization
    :param convert_srgb: False; Perform sRGB transformations (to linear and back) for correct scaling of sRGB textures
    :param is_normal_map: False; Normalize output pixels (for normal maps / encoded vectors)
    :param channel_mask: 0; Bit mask for processing a subset of channels, 0 = all channels
    :param scale_alpha_unweighted: False; Perform unweighted scaling on last channel? False = preserve coverage
    :param composite_mips: True; Composite mips from bottom to top to fill holes? Set this and scale_alpha_unweighted to False for alpha coverage preservation
    :return: list of ndarray[dtype=float]'s, or None on failure
    """
    # TODO sanity check for inputs
    image_flat = np.ascontiguousarray(image.copy())
    if image.dtype == np.uint8:
        image_data_type = DATA_TYPE.UINT8
    elif image.dtype == np.uint16:
        image_data_type = DATA_TYPE.UINT16
    elif image.dtype == np.float32:
        image_data_type = DATA_TYPE.FLOAT32
    else:
        return None

    image_buffer_pointer = c_void_p(image_flat.ctypes.data)
    image_width = image.shape[0]
    image_height = image.shape[1]
    channel_stride = image.shape[2]

    if coverage_mask is not None:
        mask_flat = np.ascontiguousarray(coverage_mask)
        mask_buffer_pointer = c_void_p(mask_flat.ctypes.data)
    else:
        mask_buffer_pointer = c_void_p(None)

    mip_count = get_mip_count(image_width, image_height)
    mips_output = (ctypes.POINTER(float_t) * mip_count)()
    masks_output = (ctypes.POINTER(uint8_t) * mip_count)()

    # generate coverage-weighted mip levels
    libmipflooding.generate_mips(
        image_buffer_pointer,
        image_data_type,
        uint16_t(image_width),
        uint16_t(image_height),
        uint8_t(channel_stride),
        mask_buffer_pointer,
        mips_output,
        masks_output,
        float_t(coverage_threshold),
        bool_t(convert_srgb),
        bool_t(is_normal_map),
        uint8_t(channel_mask),
        bool_t(scale_alpha_unweighted),
        uint8_t(max_threads))

    if composite_mips:
        # fill holes by compositing mips from bottom to top
        libmipflooding.composite_mips(
            mips_output,
            masks_output,
            uint16_t(image_width),
            uint16_t(image_height),
            uint8_t(channel_stride),
            uint8_t(channel_mask),
            uint8_t(max_threads))

    mip_width = image_width
    mip_height = image_height
    mips_list = []
    for i in range(mip_count):
        mip_width = mip_width // 2
        mip_height = mip_height // 2
        if convert_srgb:
            libmipflooding.convert_linear_to_srgb_threaded(
                uint16_t(mip_width),
                uint16_t(mip_height),
                uint8_t(channel_stride),
                mips_output[i],
                uint8_t(channel_mask),
                uint8_t(max_threads))
        np_array = np.ctypeslib.as_array(
            mips_output[i],
            [mip_height, mip_width, channel_stride]
        )
        mips_list.append(np_array.copy())

    libmipflooding.free_mips_memory(uint8_t(mip_count), mips_output, masks_output)

    return mips_list



def flood_image(
        image: np.ndarray,
        coverage_mask: Union[np.ndarray, None] = None,
        max_threads: int = 0,
        coverage_threshold: float = 0.999,
        convert_srgb: bool = False,
        is_normal_map: bool = False,
        channel_mask: int = 0,
        scale_alpha_unweighted: bool = False,
        preserve_mask_channel: bool = True
    ) -> Union[np.ndarray, None]:
    """Mip-flooded texture

    Generates a mip-flooded texture by generating and compositing coverage scaled mips, and then compositing the unmodified original image on top

    :param image: ndarray[ndim=3], types uint8 / uint16 / float
    :param coverage_mask: None; ndarray[ndim=1 or 2], type uint8, linear. If None, uses last channel of image as coverage mask.
    :param max_threads: 0; Number of threads, 0 = auto (half of available threads, which amounts to number of hardware cores for machines with SMT/HyperThreading)
    :param coverage_threshold: 0.999; Threshold for mask binarization
    :param convert_srgb: False; Perform sRGB transformations (to linear and back) for correct scaling of sRGB textures
    :param is_normal_map: False; Normalize output pixels (for normal maps / encoded vectors)
    :param channel_mask: 0; Bit mask for processing a subset of channels, 0 = all channels
    :param scale_alpha_unweighted: False; Perform unweighted scaling on last channel? False = preserve coverage
    :param preserve_mask_channel: When no mask is provided, preserve original alpha channel in output?
    :return: ndarray[ndim=3] of same type as input, or None on failure
    """
    # TODO sanity check for inputs
    image_flat = np.ascontiguousarray(image.copy())
    if image.dtype == np.uint8:
        image_data_type = DATA_TYPE.UINT8
    elif image.dtype == np.uint16:
        image_data_type = DATA_TYPE.UINT16
    elif image.dtype == np.float32:
        image_data_type = DATA_TYPE.FLOAT32
    else:
        return None

    image_buffer_pointer = c_void_p(image_flat.ctypes.data)
    image_width = image.shape[0]
    image_height = image.shape[1]
    channel_stride = image.shape[2]

    if coverage_mask is not None:
        mask_flat = np.ascontiguousarray(coverage_mask.copy())
        if image.dtype == np.uint8:
            mask_data_type = DATA_TYPE.UINT8
        elif image.dtype == np.uint16:
            mask_data_type = DATA_TYPE.UINT16
        elif image.dtype == np.float32:
            mask_data_type = DATA_TYPE.FLOAT32
        else:
            return None
        mask_buffer_pointer = c_void_p(mask_flat.ctypes.data)
    else:
        mask_data_type = image_data_type
        mask_buffer_pointer = c_void_p(None)
        if preserve_mask_channel:
            channel_mask = (channel_mask & ~(1 << channel_stride - 1)) if channel_mask != 0 else (255 >> (8 - channel_stride + 1))

    libmipflooding.flood_image(
        image_buffer_pointer,
        image_data_type,
        uint16_t(image_width),
        uint16_t(image_height),
        uint8_t(channel_stride),
        mask_buffer_pointer,
        mask_data_type,
        float_t(coverage_threshold),
        bool_t(convert_srgb),
        bool_t(is_normal_map),
        uint8_t(channel_mask),
        bool_t(scale_alpha_unweighted),
        uint8_t(max_threads))

    return image_flat.reshape(image.shape[0], image.shape[1], image.shape[2])
