print("Hello, world!")
if 5 > 2:
    print("WHAT THE FUCK AM I DOING WITH MY LIFE")
    x = 5
    y = "Jhon"
    print(x)
    print(y)
    z = 7.5
    k = int(z)
    print(k)
mylist = [1, 2, "fuck"]
for x in mylist:
    print(x)
if "fucks" in mylist:
    print("there was a fuck in there after all!")
print("list has " + str(len(mylist)) + " members")
mylist.append("no fuck")
print("fancier way: %02d" % len(mylist))
print(f"ridiculous {len(mylist)}!")
mylist.insert(0, "first")
print(mylist)
mylist.remove("no fuck")
print(mylist)

y = "test"
print(y)

class Sheep:
    x = 20
    def __init__(self):
        self.y = 30

sheep = Sheep()
print(sheep.y)

def function(x):
    print("sideeffect!")
    return x+10
fancy = lambda x : x+20

print(function(200))
print(fancy(200))
