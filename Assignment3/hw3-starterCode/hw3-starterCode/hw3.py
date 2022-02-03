import sys
import numpy as np
import matplotlib.pyplot as plt
import typing
from enum import Enum, auto

EPSILON = 0.0001
LARGE_NUM = 999999


def normalize(v):
    norm=np.linalg.norm(v, ord=1)
    if norm==0:
        norm=np.finfo(v.dtype).eps
    return v/norm


def doubleArea( a, b, c):
  return (b[0]-a[0])*(c[1]-a[1])-(c[0]-a[0])*(b[1]-a[1])

class HittingType(Enum):
    TRIANGLE = auto()
    SPHERE = auto()
    NOTHING = auto()
    
class Hit:
    ID = -1
    hittingType = HittingType.NOTHING
    hittingPoint = np.zeros(3)
    interpolation = np.ones(3)/3
    obj = None

    def getNormalAtHittingPoint(self):
        if self.hittingType == HittingType.SPHERE:
            return normalize(self.hittingPoint - self.obj.position)
        elif self.hittingType == HittingType.TRIANGLE:
            rc = np.mat(self.interpolation) * np.mat([self.obj.v[0].normal,self.obj.v[1].normal,self.obj.v[2].normal])
            rc = np.squeeze(np.asarray(rc))
            return rc

    def getShadingParameters(self):
        ks = np.zeros(3)
        kd = np.zeros(3)
        alpha = 0
        if self.hittingType == HittingType.SPHERE:
            ks = np.copy(self.obj.color_specular)
            kd = np.copy(self.obj.color_diffuse)
            alpha = self.obj.shininess
        elif self.hittingType == HittingType.TRIANGLE:
            interC = np.mat(self.interpolation)
            ks = interC * np.mat([self.obj.v[0].color_specular,self.obj.v[1].color_specular,self.obj.v[2].color_specular])
            ks = np.squeeze(np.asarray(ks))
            kd = interC * np.mat([self.obj.v[0].color_diffuse,self.obj.v[1].color_diffuse,self.obj.v[2].color_diffuse])
            kd = np.squeeze(np.asarray(kd))
            alpha = np.dot(self.interpolation, np.array([self.obj.v[0].shininess,self.obj.v[1].shininess,self.obj.v[2].shininess]))
        return kd,ks,alpha

class Vertex:
    color_diffuse = np.zeros(3)
    color_specular = np.zeros(3)
    position = np.zeros(3)
    normal = np.zeros(3)
    shininess = 0.0

class Triangle:
    ID = -1
    v: typing.List[Vertex] = list()


class Sphere:
    ID = -1
    color_diffuse = np.zeros(3)
    color_specular = np.zeros(3)
    position = np.zeros(3)
    shininess = 0.0
    radius = 0.0

class Light:
    ID = -1
    position = np.zeros(3)
    color = np.zeros(3)

class Scene:
    numGeometries = 0
    triangles: typing.List[Triangle] = list()
    spheres: typing.List[Sphere] = list()
    lights: typing.List[Light] = list()
    camera = np.zeros(3)
    ambient_light = np.zeros(3)
    fov = 60

    canvasWidth = 640
    canvasHeight = 480
    buff = np.zeros((0,0,3))
    

    def readVec3FromLine(self,s):
        return  np.array( [float(i) for i in s.split()[1:] ])
    
    def readFloatFromLine(self,s):
        return float( s.split()[1] )

    def read_sphere(self,f,ID):
        s = Sphere()
        s.ID = ID
        s.position = self.readVec3FromLine(f.readline())
        s.radius = self.readFloatFromLine(f.readline())
        s.color_diffuse = self.readVec3FromLine(f.readline())
        s.color_specular = self.readVec3FromLine(f.readline())
        s.shininess = self.readFloatFromLine(f.readline())
        self.spheres.append(s)

    def read_triangle(self,f,ID):
        t = Triangle()
        t.ID = ID
        for i in range(3):
            temp_v = Vertex()
            temp_v.position = self.readVec3FromLine(f.readline())
            temp_v.normal = self.readVec3FromLine(f.readline())
            temp_v.color_diffuse = self.readVec3FromLine(f.readline())
            temp_v.color_specular = self.readVec3FromLine(f.readline())
            temp_v.shininess = self.readFloatFromLine(f.readline())
            t.v.append(temp_v)
        self.triangles.append(t)

    def read_light(self,f,ID):
        l = Light()
        l.ID = ID
        l.position = self.readVec3FromLine(f.readline())        
        l.color = self.readVec3FromLine(f.readline())
        self.lights.append(l)

    def __init__(self,filename):
        f = open(filename,"r")
        if not f: exit()
        self.numGeometries = int(f.readline())
        self.ambient_light = self.readVec3FromLine(f.readline())
        ID = 0
        while True:
            a = f.readline().strip()
            if a == '': break
            if a in ['sphere', 'triangle','light']:
                eval("self.read_{}(f,ID)".format(a))
                ID += 1


    def rayHitsSphere(self,r0,rd, s: Sphere):
        b = 2 * np.dot(rd, r0-s.position)
        c = np.sum( (r0-s.position)**2) - s.radius**2
        delta = b**2 - 4*c
        if delta<0: return False,LARGE_NUM,LARGE_NUM
        t1 = (-b - np.sqrt(delta)) / 2
        t2 = (-b + np.sqrt(delta)) / 2
        if t1<EPSILON: t1 = LARGE_NUM
        if t2<EPSILON: t2 = LARGE_NUM
        return (t1>EPSILON or t2>EPSILON), t1,t2

    def pointInTriangle(self,C,C0,C1,C2):
        c,c0,c1,c2 = C[:2],C0[:2],C1[:2],C2[:2]
        denom = doubleArea(c0,c1,c2)
        if denom == 0:
            c,c0,c1,c2 = C[1:],C0[1:],C1[1:],C2[1:]
            denom = doubleArea(c0,c1,c2)
            if denom == 0:
                c,c0,c1,c2 = C[[0,2]],C0[[0,2]],C1[[0,2]],C2[[0,2]]
                denom = doubleArea(c0,c1,c2)
                if denom == 0:
                    print("1!!!!!")
                    return c == c0, np.array([1/3,1/3,1/3])
        alpha = doubleArea(c,c1,c2) / denom
        beta = doubleArea(c0,c,c2) / denom
        gamma = 1 - alpha - beta
        return (0 < alpha and alpha < 1 and 0 < beta and beta < 1 and 0 < gamma and gamma < 1), np.array([alpha,beta,gamma])

    def rayHitsTriangle(self,r0,rd,tri:Triangle):
        c0,c1,c2 = np.copy(tri.v[0].position), np.copy(tri.v[1].position), np.copy(tri.v[2].position)
        normal = normalize(np.cross(c1-c0,c2-c0))
        normalDotRd = np.dot(normal,rd)
        if normalDotRd == 0: return False,LARGE_NUM,None
        t = -(np.dot(r0,normal) - np.dot(normal,c0))/normalDotRd;
        if t<EPSILON: return False,LARGE_NUM,None
        hittingPoint =  r0 + t*rd
        rc, interCoord = self.pointInTriangle(np.copy(hittingPoint),c0,c1,c2)
        return rc,t,interCoord
    
    def point2LightNotBlocked(self,p, l:Light, objID = -1):
        rd = normalize(l.position - p)
        for i in self.spheres:
            if objID == i.ID: continue
            rc, _ , _  = self.rayHitsSphere(p,rd,i)
            if rc: return False
        for i in self.triangles:
            if objID == i.ID: continue
            rc, _ , _ = self.rayHitsTriangle(p,rd,i)
            if rc: return False
        return True

    def rayHits(self,r0,rd):
        minT = LARGE_NUM
        hitting = Hit()
        for i in self.spheres:
            rc,t1,t2 = self.rayHitsSphere(r0,rd,i)
            if rc: 
                if t1<minT or t2 < minT:
                    hitting.hittingType = HittingType.SPHERE
                    hitting.ID = i.ID
                    hitting.obj = i
                    minT = min(t1,t2)    
        for i in self.triangles:
            rc,t,interCoord = self.rayHitsTriangle(r0,rd,i)
            if rc and t < minT:
                minT = t
                hitting.hittingType = HittingType.TRIANGLE
                hitting.obj = i
                hitting.interpolation = interCoord
                hitting.ID = i.ID           
        if hitting.ID != -1:
            hitting.hittingPoint = r0 + minT * rd
            # print(rd,hitting.ID,hitting.hittingType,hitting.hittingPoint)
        return hitting


    def shadingAtHit(self,h:Hit):
        color = np.copy(self.ambient_light)
        kd,ks,alpha = h.getShadingParameters()
        n = h.getNormalAtHittingPoint()
        v = normalize(self.camera - h.hittingPoint)
        for i in self.lights:
            l = normalize(i.position - h.hittingPoint)
            r = normalize(2*np.dot(l,n)*n-l);
            if self.point2LightNotBlocked(np.copy(h.hittingPoint),i,objID=h.ID):
                color += i.color * ( kd * max(0,np.dot(l,n)) + ks * (max(0,np.dot(v,r)) ** alpha)  )
            
        return color
        
    def rayTracingRendering(self):
        self.buff = np.zeros((self.canvasHeight,self.canvasWidth,3))
        tanHalfFov = np.tan( self.fov * ( np.pi / 180.0 ) / 2.0)
        aspectRatio = self.canvasWidth / self.canvasHeight
        dx = 2*aspectRatio*tanHalfFov/self.canvasWidth;
        dy = 2*tanHalfFov/self.canvasHeight
        for i in range(self.canvasHeight):
            for j in range(self.canvasWidth):
                ray =  normalize(np.array([-aspectRatio*tanHalfFov + j*dx, -tanHalfFov + i*dy, -1]))
                h = self.rayHits(self.camera,ray)
                if h.ID != -1:  
                    self.buff[i][j] = self.shadingAtHit(h)
                    # print(i,j, self.buff[i][j])
                    # print()
                else: self.buff[i][j] = np.ones(3)/20
        

    def display(self,displayWidth, displayHeight):
        self.canvasHeight = displayHeight
        self.canvasWidth = displayWidth
        self.buff = np.zeros((self.canvasHeight,self.canvasWidth,3))
        self.camera = np.zeros(3)
        self.rayTracingRendering()
        plt.imshow(self.buff)
        plt.show()


    
if __name__ == "__main__":
    s = Scene(sys.argv[1])
    s.display(128,96)

    # s.rayHisTriangle(np.array([0,0,0]),np.array([0,-1,-1]),Triangle())