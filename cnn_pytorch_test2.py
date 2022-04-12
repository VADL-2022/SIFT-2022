# Based on https://github.com/python-engineer/pytorchTutorial/blob/master/14_cnn.py , https://www.youtube.com/watch?v=pDdP0TFzsoQ&list=PLqnslRFeH2UrcDBWF5mfPGpqQDSta6VK4&index=15

import torch
import torch.nn as nn
import torch.nn.functional as F
import torchvision
import torchvision.transforms as transforms
import matplotlib.pyplot as plt
import numpy as np

# Device configuration
device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

# Hyper-parameters 
num_epochs = 5
batch_size = 4
learning_rate = 0.001

# dataset has PILImage images of range [0, 1]. 
# We transform them to Tensors of normalized range [-1, 1]
transform = transforms.Compose(
    [transforms.ToTensor(),
     transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5))])

# CIFAR10: 60000 32x32 color images in 10 classes, with 6000 images per class
train_dataset = torchvision.datasets.CIFAR10(root='./data', train=True,
                                        download=True, transform=transform)

test_dataset = torchvision.datasets.CIFAR10(root='./data', train=False,
                                       download=True, transform=transform)

train_loader = torch.utils.data.DataLoader(train_dataset, batch_size=batch_size,
                                          shuffle=True)

test_loader = torch.utils.data.DataLoader(test_dataset, batch_size=batch_size,
                                         shuffle=False)

classes = ('plane', 'car', 'bird', 'cat',
           'deer', 'dog', 'frog', 'horse', 'ship', 'truck')

def imshow(img):
    img = img / 2 + 0.5  # unnormalize
    npimg = img.numpy()
    plt.imshow(np.transpose(npimg, (1, 2, 0)))
    plt.show()


# get some random training images
dataiter = iter(train_loader)
images, labels = dataiter.next()

# show images
imshow(torchvision.utils.make_grid(images))

# 
# /** @brief keypoint structure, related to a keypoint
#  *
#  * stores SIFT output (keypoint position, scale, orientation, feature vector...)
#  * stores intermediary results (orientation histogram, summarizes...)
#  *
#  *
#  */
# struct keypoint
# {
#     float x; // coordinates
#     float y;
#     float sigma; // level of blur (it includes the assumed image blur)
#     float theta; // orientation
# 
#     int o; // discrete coordinates
#     int s;
#     int i;
#     int j;
# 
#     float val; // normalized operator value (independant of the scalespace sampling)
#     float edgeResp; // edge response
#     int n_hist;     // number of histograms in each direction
#     int n_ori;      // number of bins per histogram
#     float* descr;
# 
#     int n_bins; // number of bins in the orientation histogram
#     float* orihist; // gradient orientation histogram
# };
# 
#numChannelsPerPixel = 3 # Let `int l = kB->n_hist * kB->n_hist * kB->n_ori; // length of the feature vector`     . then: numChannelsPerPixel is: x, y, sigma, theta, descr[`l`], orihist[`l`] of each keypoint

# int n_hist,
# int n_ori,
# int n_bins,
# int flag
# n_hist=4
# n_ori=8
# n_bins=36
# flag=2

# l = n_hist * n_hist n_ori
# numChannelsPerPixel = (  1 # x
#                        + 1 # y
#                        + 1 # sigma
#                        + 1 # theta
#                        + l # descr
#                        + l # orihist
#                        )
# ^^nvm the above, it's simpler because matches are the only ones given to the CNN - change of plans:
numChannelsPerPixel = (  1 # x
                       + 1 # y
                       )
print("numChannelsPerPixel:", numChannelsPerPixel)

class ConvNet(nn.Module):
    def __init__(self):
        super(ConvNet, self).__init__()
        # CREATE LAYERS:
        # Define/create the first convolution layer
        outputConv1ChannelSize=6
        self.conv1 = nn.Conv2d(numChannelsPerPixel # input channel size
                               , outputConv1ChannelSize # output channel size. Choose whatever you want.
                               , 5 # kernel size (5 would mean 5x5 is the number of elements in the kernel) -- the kernel moves over the "image" (matrix) and convolutes them or whatever. this reduces/changes the amount of data.
                               )
        # Define a pooling layer
        self.pool = nn.MaxPool2d(2 # kernel size
                                 , 2 # stride: after each convolution operation using the kernel size, shift the kernel by this amount to the right.
                                 )
        # Define the second convolution layer
        self.conv2 = nn.Conv2d(outputConv1ChannelSize # Must equal the *last output channel size* (the last conv2d output channel size)
                               , 16 # Output channel size
                               , 5 # kernel size
                               )
        # Convolution layers are done.
        # Now set up the fully-connected layers:
        outputFeatures1=120
        self.fc1 = nn.Linear(16 * 5 * 5
                             , outputFeatures1 # number of output features. can be tweaked
                             )
        # Second fully-connected layers
        outputFeatures2=84
        self.fc2 = nn.Linear(outputFeatures1 # `outputFeatures` is now used as the number of input features to the next fully-connected layer.
                             , outputFeatures2 # number of output features
                             )
        self.fc3 = nn.Linear(outputFeatures2
                             , 10 # Output size must be the number of classes to classify into.
                             )

    def forward(self, x):
        # -> n, 3, 32, 32
        x = self.pool(F.relu(self.conv1(x)))  # -> n, 6, 14, 14
        x = self.pool(F.relu(self.conv2(x)))  # -> n, 16, 5, 5
        x = x.view(-1, 16 * 5 * 5)            # -> n, 400
        x = F.relu(self.fc1(x))               # -> n, 120
        x = F.relu(self.fc2(x))               # -> n, 84
        x = self.fc3(x)                       # -> n, 10
        return x


model = ConvNet().to(device)

criterion = nn.CrossEntropyLoss()
optimizer = torch.optim.SGD(model.parameters(), lr=learning_rate)

n_total_steps = len(train_loader)
for epoch in range(num_epochs):
    for i, (images, labels) in enumerate(train_loader):
        # origin shape: [4, 3, 32, 32] = 4, 3, 1024
        # input_layer: 3 input channels, 6 output channels, 5 kernel size
        images = images.to(device)
        labels = labels.to(device)

        # Forward pass
        outputs = model(images)
        loss = criterion(outputs, labels)

        # Backward and optimize
        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        if (i+1) % 2000 == 0:
            print (f'Epoch [{epoch+1}/{num_epochs}], Step [{i+1}/{n_total_steps}], Loss: {loss.item():.4f}')

print('Finished Training')
PATH = './cnn.pth'
torch.save(model.state_dict(), PATH)

with torch.no_grad():
    n_correct = 0
    n_samples = 0
    n_class_correct = [0 for i in range(10)]
    n_class_samples = [0 for i in range(10)]
    for images, labels in test_loader:
        images = images.to(device)
        labels = labels.to(device)
        outputs = model(images)
        # max returns (value ,index)
        _, predicted = torch.max(outputs, 1)
        n_samples += labels.size(0)
        n_correct += (predicted == labels).sum().item()
        
        for i in range(batch_size):
            label = labels[i]
            pred = predicted[i]
            if (label == pred):
                n_class_correct[label] += 1
            n_class_samples[label] += 1

    acc = 100.0 * n_correct / n_samples
    print(f'Accuracy of the network: {acc} %')

    for i in range(10):
        acc = 100.0 * n_class_correct[i] / n_class_samples[i]
        print(f'Accuracy of {classes[i]}: {acc} %')
