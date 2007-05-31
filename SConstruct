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
qtenv = Environment(tools=['default', 'qt4', 'svninfo'], toolpath=['#'])

# get the location of qwt
qtenv['QWTDIR'] = os.environ.get('QWTDIR', None)
#  add include path for qwt
qtenv.AppendUnique(CPPPATH=['$QWTDIR/include',])

# get the location of qttoolbox
qtenv['QTTOOLBOXDIR'] = os.environ.get('QTTOOLBOXDIR', None)
# add include path to all of the QtToolbox components
# (check out the interesting for loop that executes qtenv.AppendUnique()
toolboxdirs = ['ColorBar', 'Knob', 'TwoKnobs', 'ScopePlot', 'PPI']
x = [qtenv.AppendUnique(CPPPATH=['$QTTOOLBOXDIR','$QTTOOLBOXDIR/'+dir,]) for dir in toolboxdirs]

# get the location of installed fftw
qtenv['FFTWDIR'] = os.environ.get('FFTWDIR', None)
#  add include path for fftw
qtenv.AppendUnique(CPPPATH=['$FFTWDIR/include',])

# get the location of installed glut
qtenv['GLUTDIR'] = os.environ.get('GLUTDIR', None)
#  add include path for glut
qtenv.AppendUnique(CPPPATH=['$GLUTDIR/GL/include',])

# Add include path to the main cp2 libraries
qtenv.AppendUnique(CPPPATH=['#./CP2Net','#./Moments','#./CP2Config','#./CP2Lib'])

# Enable any qt module that is used in cp2
qtenv.EnableQt4Modules(['QtCore','QtGui','QtOpenGL','QtNetwork'])

# Export this fancy environment
Export('qtenv')

# Build cp2 components
cp2dirs = [
'Moments',
'CP2Config',
'CP2Lib',
'CP2Net',
'CP2Moments',
'CP2Scope',
'CP2PPI'
]

x = [SConscript(dir+'/SConscript') for dir in cp2dirs]
