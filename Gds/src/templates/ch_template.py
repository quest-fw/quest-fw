'''
Created on Apr. 27, 2015

@author: reder
'''

import data_template

from serializable.base_type import *


class Channel(data_template.DataTemplate):
    '''
    Channel class is for deserialize channel telemetry value.
    Really this is a container since the type will have it's own deserialize
    implementation.
    '''
    def __init__(self, ch_id, ch_name, comp_name, ch_desc, ch_type_obj,
                 ch_format_str, low_red, low_orange, low_yellow, high_yellow,
                 high_orange, high_red):
        '''
        Constructor

        Args:
            ch_id: The ID of the channel being described
            ch_name: The name of the channel
            comp_name: The name of the f-prime component that produces this ch
            ch_desc: Description of the channel
            ch_type_obj: The channel's type as an instance of a class derived
                         from BaseType
            ch_format_str: The format string for the channel (may be None)
            low_red: (Optional) Below this the value will be in red alert
            low_orange: (Optional) Below this the value will be in orange alert
            low_yellow: (Optional) Below this the value will be in yellow alert
            high_yellow: (Optional) Above this the value will be in yellow alert
            high_orange: (Optional) Above this the value will be in orange alert
            high_red: (Optional) Above this the value will be in red alert
        '''
        # Make sure correct types are passed
        # TODO do we need to do this check
        if not type(ch_id) == type(int()):
            raise TypeMismatchException(type(int()),type(ch_id))

        if not type(ch_name) == type(str()):
            raise TypeMismatchException(type(str()), type(ch_name))

        if not type(ch_desc) == type(str()):
            raise TypeMismatchException(type(str()), type(ch_desc))

        if not issubclass(type(ch_type), type(BaseType())):
            raise TypeMismatchException(type(BaseType()),type(ch_type))

        # Initialize event internal variables
        self.id          = ch_id
        self.name        = ch_name
        self.comp_name   = comp_name
        self.ch_desc     = ch_desc
        self.ch_type_obj = ch_type_obj
        self.format_str  = ch_format_str
        self.low_red     = low_red
        self.low_orange  = low_orange
        self.low_yellow  = low_yellow
        self.high_yellow = high_yellow
        self.high_orange = high_orange
        self.high_red    = high_red


    def get_comp_name(self):
        return self.comp_name

    def get_ch_desc(self):
        return self.ch_desc

    def get_type_obj(self):
        return self.ch_type_obj

   def get_format_string(self):
        return self.format_string

    def get_low_red(self):
        return self.low_red

    def get_low_orange(self):
        return self.low_orange

    def get_low_yellow(self):
        return self.low_yellow

    def get_high_yellow(self):
        return self.high_yellow

    def get_high_orange(self):
        return self.high_orange

    def get_high_red(self):
        return self.high_red


if __name__ == '__main__':
    pass

