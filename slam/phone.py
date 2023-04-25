import cv2 as cv2
import numpy as np

cap = cv2.VideoCapture('http://192.168.1.33:8080/video')

K = [[1685.9, 0, 960],
     [0, 1691.3, 540],
     [0,      0,   1]]
K = np.array(K)

distCoff = [0.3011, -1.3315, 0.001942, 0.00971, 1.84797]
distCoff = np.array(distCoff)

print(f"K: {K}")

orb = cv2.ORB_create()
bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=False)

lastKp = None
lastDes = None
lastFrame = None

lastNormMatches = None

def extractKeypoints(img):
    # find features
    corners = cv2.goodFeaturesToTrack(img, 1000, 0.0008, 10)
    # compute keypoints with descriptors
    kp = [cv2.KeyPoint(x=f[0][0], y=f[0][1], _size=20) for f in corners]
    kp, des = orb.compute(img, kp)
    return kp,des

def findMatches(des1, kp1, des2, kp2):
    matches = bf.knnMatch(des1, des2, k=2)
    good = [m for m,n in matches if m.distance < 0.7*n.distance]
    return [(kp1[m.queryIdx].pt, kp2[m.trainIdx].pt) for m in good]

while(True):
    ret, frame = cap.read()
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    kp, des = extractKeypoints(gray)
    if lastKp is not None:
        matches = findMatches(lastDes, lastKp, des, kp)
        print(f"{len(matches)} matches")
        pt1, pt2 = zip(*matches)
        pt1 = np.array(pt1)
        pt2 = np.array(pt2)

        gp1 = []
        gp2 = []

        # filter using fundamental matrix

        rejected = 0
        F, mask = cv2.findFundamentalMat(pt1, pt2, cv2.FM_RANSAC, 1.0, 0.999, 1000)
        print(F)
        if F is not None:
            for p1,p2,m in zip(pt1, pt2, mask):
                if m >= 1.0:
                    p2x = F.dot(np.append(p1, 1.0))
                    print(f"original {p2}")
                    p2 = np.array((p2x[0]/p2x[2], p2x[1]/p2x[2]))
                    print(f"new {p2}")
                    cv2.line(frame, (int(p1[0]),int(p1[1])), (int(p2[0]),int(p2[1])), (255,0,0), 1)
                    gp1.append(p1)
                    gp2.append(p2)
                else:
                    rejected += 1
        print(f"rejected count: {rejected}")
        gp1 = np.array(gp1)
        gp2 = np.array(gp2)
        assert len(gp1) == len(gp2)

        if len(gp1) > 5:
            pt1U = cv2.undistortPoints(gp1, K, None)
            pt2U = cv2.undistortPoints(gp2, K, None)
            pt1U = pt1U[:,0]
            pt2U = pt2U[:,0]
            print(pt1U)

            E, mask = cv2.findEssentialMat(pt1U, pt2U, K, cv2.RANSAC, 0.999, 0.005, 1000)
            print(f"E: {E}")

    #for corner in corners:
        #x,y = corner.ravel()
        #cv2.circle(frame,(x,y),6,(0,0,255),-1)

    #orb = cv2.ORB_create()
    #kp = orb.detect(gray, None)
    #kp, des = orb.compute(gray, kp)

    frame = cv2.drawKeypoints(frame, kp, None, color=(0,0,255), flags=0)


    lastKp = kp
    lastFrame = gray
    lastDes = des

    cv2.imshow('frame', frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
