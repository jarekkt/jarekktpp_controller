import matplotlib.pyplot as plt
import array as arr
import math
import numpy as np


class tick:
    def __init__(self):
        self.plt      = plt
        self.m_dist   = 2000.0
        self.m_speed  = 800.0
        self.m_speed0 = 00.0
        self.m_accel  = 1000.0
        self.m_accel_s = 2*self.m_accel
        self.m_jerk   = 100.0
        self.m_jerk_used = self.m_jerk
        self.m_tick   = 0.00001
        self.c_speed  = self.m_speed0
        self.c_accel  = 0
        self.c_jerk   = 0
        self.c_dist   = 0
        self.c_tick   = 0
        self.c_idx    = 0

        self.t_dist   = arr.array('f')
        self.t_speed  = arr.array('f')
        self.t_accel  = arr.array('f')
        self.t_jerk   = arr.array('f')
        self.t_time   = arr.array('f')


    def calc_phase(self):
        hd = self.m_dist / 2
        # hd = m_speed0 * T + 0.5 * m_accel * T^2
        # T->   (0.5 * m_accel ) * T^2 + (m_speed0) * T + (-hd) = 0
        delta = self.m_speed0 * self.m_speed0 - 4 * 0.5 * self.m_accel * -hd
        T = (-self.m_speed0 + math.sqrt(delta)) / (2* (0.5 * self.m_accel))

        if self.m_speed0 + self.m_accel * T > self.m_speed:
            #Acceleration phase + constant speed phase
            self.T1 = (self.m_speed - self.m_speed0)/self.m_accel
            self.T1_dist = self.m_speed0 * self.T1 + self.m_accel * self.T1*self.T1/2
            self.T3 = self.T1
            self.T3_dist = self.T1_dist
            self.T2_dist = self.m_dist - (self.T1_dist + self.T3_dist)
            self.T2 = self.T2_dist/self.m_speed
        else:
            #Single accelerated phase
            self.T1 = T
            self.T1_dist = self.m_speed0 * self.T1 + self.m_accel * self.T1 * self.T1 / 2
            self.T2 = 0.0
            self.T2_dist = 0.0
            self.T3 = T
            self.T3_dist = self.T1_dist

        print( 'Trapezoid T1 {}/{} T2 {}/{} T3 {}/{}'.format(self.T1,self.T1_dist,self.T2,self.T2_dist,self.T3,self.T3_dist))
        print('Total {}/{}'.format(self.m_dist,self.T1_dist+self.T2_dist+self.T3_dist))



        if self.m_accel_s > 2* self.m_accel:
            self.m_accel_s = 2 * self.m_accel
            self.m_jerk_used = 2 * self.m_accel_s / self.T1
        else:
            self.m_jerk_used = self.m_accel_s * self.m_accel_s / (self.m_speed - self.m_speed0)

        self.T11 = self.m_accel_s/self.m_jerk_used
        self.T12 = self.T1 - 2* self.T11
        self.T13 = self.T11

        print( 'S phases T11 {} T12 {} T13 {}'.format(self.T11,self.T12,self.T13))



    def one_step(self,mode):

        if mode == 0:
            if self.c_tick < self.T1:
                self.c_accel = self.m_accel
                self.c_speed = self.c_speed + self.c_accel * self.m_tick
            elif self.c_tick < self.T2 + self.T1:
                pass
            else:
                self.c_speed = self.c_speed - self.c_accel * self.m_tick

        if mode == 1:

            if self.c_tick < self.T1:
                if self.c_tick < self.T11:
                    self.c_jerk = self.m_jerk_used
                elif self.c_tick < self.T11+self.T12:
                    self.c_jerk  = 0
                    self.c_accel = self.m_accel_s
                else:
                    self.c_jerk = -self.m_jerk_used

                self.c_accel = self.c_accel + self.c_jerk * self.m_tick
                self.c_speed = self.c_speed + self.c_accel * self.m_tick

            elif self.c_tick < self.T2 + self.T1:
                self.c_speed = self.m_speed
                self.c_jerk  = 0
            else:
                if self.c_tick < self.T2 + self.T1+self.T11:
                    self.c_jerk = -self.m_jerk_used
                elif self.c_tick < self.T2 + self.T1 + self.T11+self.T12:
                    self.c_jerk = 0
                    self.c_accel = -self.m_accel_s
                else:
                    self.c_jerk = +self.m_jerk_used

                self.c_accel = self.c_accel + self.c_jerk * self.m_tick
                self.c_speed = self.c_speed + self.c_accel * self.m_tick

        self.c_dist = self.c_dist + self.c_speed * self.m_tick


    def execute(self):
        self.calc_phase()

        while self.m_dist >= self.c_dist:
            self.one_step(1)
            self.add_log()
            self.advance()

    def  add_log(self):
        self.t_jerk.append(self.c_jerk)
        self.t_speed.append(self.c_speed)
        self.t_accel.append(self.c_accel)
        self.t_time.append(self.c_tick)
        self.t_dist.append(self.c_dist)

    def advance(self):
        self.c_idx = self.c_idx + 1
        self.c_tick = self.c_tick + self.m_tick

    def plot(self):
        # Plot the data
        self.plt.subplot(4, 1, 1)
        self.plt.plot(self.t_time, self.t_dist, label='distance')
        self.plt.legend()

        self.plt.subplot(4, 1, 2)
        self.plt.plot(self.t_time, self.t_speed, label='speed')
        self.plt.legend()

        self.plt.subplot(4, 1, 3)
        self.plt.plot(self.t_time, self.t_accel, label='acceleration')
        self.plt.legend()

        self.plt.subplot(4, 1, 4)
        self.plt.plot(self.t_time, self.t_jerk, label='jerk')
        self.plt.legend()

        # Show the plot
        self.plt.subplots_adjust(hspace=0.4)
        self.plt.show()

tt = tick()
tt.execute()
tt.plot()
print('Done')

