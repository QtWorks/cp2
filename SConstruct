import os

# The following environment variables are expected:
# QTDIR        - The location of Qt4
# QWTDIR       - The location of the qwt installation
# QTTOOLBOXDIR - The location of the QtToolbox installation.
# FFTWDIR      - The location of installed fftw. A lib and include directory 
#                are expected below this
# GLUTDIR      - The location of glut.
#
# Create an environment for building Qt and QtToolbox apps.
# We will add include paths here, but not librariy paths 
# or libraries. Individual components will need to specify
# these either by adding them to the imported environment
# or when invoking the builders.

# the qt4 tool will be found in the top directory
env = Environment(tools=['default', 'qt4', 'svninfo'], toolpath=['#'])

#  add our ideas for CCFLAGS
env.AppendUnique(CCFLAGS=['-O2',])

# get the location of qwt
env['QWTDIR'] = os.environ.get('QWTDIR', None)
#  add include path for qwt
env.AppendUnique(CPPPATH=['$QWTDIR/include',])

# get the location of qttoolbox
env['QTTOOLBOXDIR'] = os.environ.get('QTTOOLBOXDIR', None)
# add include path to all of the QtToolbox components
# (check out the interesting for loop that executes env.AppendUnique()
toolboxdirs = ['ColorBar', 'Knob', 'TwoKnobs', 'ScopePlot', 'PPI']
x = [env.AppendUnique(CPPPATH=['$QTTOOLBOXDIR','$QTTOOLBOXDIR/'+dir,]) for dir in toolboxdirs]

# get the location of installed fftw
env['FFTWDIR'] = os.environ.get('FFTWDIR', None)
#  add include path for fftw
env.AppendUnique(CPPPATH=['$FFTWDIR/include',])

# get the location of installed glut
env['GLUTDIR'] = os.environ.get('GLUTDIR', None)
#  add include path for glut
env.AppendUnique(CPPPATH=['$GLUTDIR/GL/include',])

# add include path to the main cp2 libraries
env.AppendUnique(CPPPATH=['#./CP2Net','#./Moments','#./CP2Config','#./CP2Lib'])

# Enable any qt module that is used in cp2
env.EnableQt4Modules(['QtCore','QtGui','QtOpenGL','QtNetwork'])

# add an install target method for applications
def InstallBin(self, bin):
	installdir = '#/bin'
	self.Install(installdir, bin)
	self.Alias('install', installdir)

Environment.InstallBin = InstallBin

# Export this fancy environment
Export('env')

# Build cp2 components
cp2dirs = [
'Moments',
'CP2Config',
'CP2Lib',
'CP2Net',
'CP2Moments',
'CP2Scope',
'CP2PPI',
'CP2Sim'
]

x = [SConscript(dir+'/SConscript') for dir in cp2dirs]
